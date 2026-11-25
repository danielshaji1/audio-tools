// SPDX-License-Identifier: GPL-3.0-or-later
// Build reportsettings.cpp with SecItem* mapped to the in-memory functions below.
#include "../src/service/reportsettings.h"
#include <QCoreApplication>
#include <Security/Security.h>
#include <cassert>

static CFDataRef stored = nullptr;
static void saveItem(CFDictionaryRef attributes) {
    auto data = static_cast<CFDataRef>(CFDictionaryGetValue(attributes, kSecValueData));
    if (stored) CFRelease(stored);
    stored = CFDataCreateCopy(nullptr, data);
}
extern "C" OSStatus mockCopy(CFDictionaryRef, CFTypeRef *result) {
    if (!stored) return errSecItemNotFound;
    *result = CFRetain(stored);
    return errSecSuccess;
}
extern "C" OSStatus mockAdd(CFDictionaryRef query, CFTypeRef *) {
    if (stored) return errSecDuplicateItem;
    saveItem(query);
    return errSecSuccess;
}
extern "C" OSStatus mockUpdate(CFDictionaryRef, CFDictionaryRef values) {
    if (!stored) return errSecItemNotFound;
    saveItem(values);
    return errSecSuccess;
}
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    ReportSettings first;
    const QString leader = "leader@example.org";
    first.save(leader,true,"123456","654321");
    assert(!first.configured());
    first.save(leader,true,"123456","123456");
    assert(first.automatic());
    ReportSettings reopened;
    assert(reopened.email()==leader && reopened.automatic());
    reopened.save("other@example.org",false,"654321","");
    assert(reopened.email()==leader && reopened.automatic());
    reopened.save("other@example.org",false,"123456","");
    ReportSettings updated;
    assert(updated.email()=="other@example.org" && !updated.automatic());
    CFRelease(stored);
}
