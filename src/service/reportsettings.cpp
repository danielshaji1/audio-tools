// SPDX-License-Identifier: GPL-3.0-or-later
#include "reportsettings.h"
#include <QtCore>
#include <QFileDialog>
#include <QPasswordDigestor>
#include <Security/Security.h>

static CFMutableDictionaryRef query() {
    auto q = CFDictionaryCreateMutable(nullptr,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(q,kSecClass,kSecClassGenericPassword);
    CFDictionarySetValue(q,kSecAttrService,CFSTR("app.lightdmx.audiotools.reporting"));
    CFDictionarySetValue(q,kSecAttrAccount,CFSTR("leader"));
    return q;
}
ReportSettings::ReportSettings() { if(!QCoreApplication::arguments().contains("--smoke-test"))reload(); }
bool ReportSettings::reload() {
    auto q=query(); CFDictionarySetValue(q,kSecReturnData,kCFBooleanTrue);
    CFTypeRef result=nullptr;
    auto status=SecItemCopyMatching(q,&result); CFRelease(q);
    accessible=status==errSecItemNotFound;
    record={};
    if(status==errSecSuccess && result) {
        auto data=static_cast<CFDataRef>(result);
        record=QJsonDocument::fromJson(QByteArray(reinterpret_cast<const char*>(CFDataGetBytePtr(data)),CFDataGetLength(data))).object();
        accessible=record.value("hash").toString().size()==64 && record.value("salt").toString().size()==32;
        CFRelease(result);
    }
    return accessible;
}
bool ReportSettings::configured() const {return accessible && !record.isEmpty();}
bool ReportSettings::automatic() const {return configured() && record.value("auto").toBool();}
QString ReportSettings::email() const {return configured()?record.value("email").toString():QSettings().value("service/email").toString();}
QString ReportSettings::pickFolder(QString current) {
    return QFileDialog::getExistingDirectory(nullptr, "Select a shared or synced report folder",current);
}
QString ReportSettings::save(QString address,bool enabled,QString pin,QString confirmation,QString folder) {
    if(!reload())return "Unlock the login Keychain to update reporting.";
    address=address.trimmed();
    if(enabled && !QRegularExpression("^[^\\s@,;<>]+@[^\\s@,;<>]+\\.[^\\s@,;<>]+$").match(address).hasMatch())return "Enter one valid recipient address.";
    if(!folder.isEmpty() && (enabled || !QFileInfo(folder).isAbsolute() || !QFileInfo(folder).isDir()))
        return "Select an existing report folder.";
    const bool existing=configured();
    if(!QRegularExpression("^[0-9]{6,12}$").match(pin).hasMatch() || (!existing && pin!=confirmation))
        return existing?"Enter the leader PIN.":"Enter the same 6–12 digit PIN twice.";
    QByteArray salt=existing?QByteArray::fromHex(record.value("salt").toString().toLatin1()):QUuid::createUuid().toRfc4122();
    auto hash=QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Sha256,pin.toLatin1(),salt,210000,32).toHex();
    if(existing && hash!=record.value("hash").toString().toLatin1())return "Incorrect leader PIN.";
    QJsonObject next{{"folder",folder},{"email",address},{"auto",enabled},{"hash",QString::fromLatin1(hash)},{"salt",QString::fromLatin1(salt.toHex())}};
    auto bytes=QJsonDocument(next).toJson(QJsonDocument::Compact);
    auto data=CFDataCreate(nullptr,reinterpret_cast<const UInt8*>(bytes.constData()),bytes.size());
    auto q=query(); OSStatus status;
    if(existing){
        auto attrs=CFDictionaryCreateMutable(nullptr,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
        CFDictionarySetValue(attrs,kSecValueData,data);
        status=SecItemUpdate(q,attrs);CFRelease(attrs);
    }else{CFDictionarySetValue(q,kSecValueData,data);status=SecItemAdd(q,nullptr);}
    CFRelease(data);CFRelease(q);
    if(status!=errSecSuccess)return "Could not save the protected settings in Keychain.";
    record=next;emit changed();return "Reporting preferences saved.";
}
