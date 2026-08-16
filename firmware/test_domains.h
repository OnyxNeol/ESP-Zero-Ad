#ifndef TEST_DOMAINS_H
#define TEST_DOMAINS_H

#include <Arduino.h>

// ============================================================================
// AdBlock Test Domains — from adblock.turtlecute.org (d3host.txt)
// ============================================================================
// These 129 domains are used to test whether the  router ad blocker service
// can block them. The test runs BEFORE the block list is created.
// Only domains the router CANNOT block get added to the ESP32-S3's
// block list.
//
// Source: https://github.com/Turtlecute33/adblocktest
// License: CC BY-NC-SA
// ============================================================================

// Category enum
enum TestCategory {
    CAT_ADS,
    CAT_ANALYTICS,
    CAT_ERROR_TRACKERS,
    CAT_SOCIAL,
    CAT_MIX,
    CAT_OEM
};

// Test domain entry
struct TestDomain {
    const char* domain;
    TestCategory category;
};

// All 129 test domains from d3host.txt
static const TestDomain TEST_DOMAINS[] = {
    // ===== Ads =====
    {"adtago.s3.amazonaws.com", CAT_ADS},
    {"analyticsengine.s3.amazonaws.com", CAT_ADS},
    {"analytics.s3.amazonaws.com", CAT_ADS},
    {"advice-ads.s3.amazonaws.com", CAT_ADS},
    {"pagead2.googlesyndication.com", CAT_ADS},
    {"adservice.google.com", CAT_ADS},
    {"pagead2.googleadservices.com", CAT_ADS},
    {"afs.googlesyndication.com", CAT_ADS},
    {"stats.g.doubleclick.net", CAT_ADS},
    {"ad.doubleclick.net", CAT_ADS},
    {"static.doubleclick.net", CAT_ADS},
    {"m.doubleclick.net", CAT_ADS},
    {"mediavisor.doubleclick.net", CAT_ADS},
    {"ads30.adcolony.com", CAT_ADS},
    {"adc3-launch.adcolony.com", CAT_ADS},
    {"events3alt.adcolony.com", CAT_ADS},
    {"wd.adcolony.com", CAT_ADS},
    {"static.media.net", CAT_ADS},
    {"media.net", CAT_ADS},
    {"adservetx.media.net", CAT_ADS},

    // ===== Analytics =====
    {"analytics.google.com", CAT_ANALYTICS},
    {"click.googleanalytics.com", CAT_ANALYTICS},
    {"google-analytics.com", CAT_ANALYTICS},
    {"ssl.google-analytics.com", CAT_ANALYTICS},
    {"adm.hotjar.com", CAT_ANALYTICS},
    {"identify.hotjar.com", CAT_ANALYTICS},
    {"insights.hotjar.com", CAT_ANALYTICS},
    {"script.hotjar.com", CAT_ANALYTICS},
    {"surveys.hotjar.com", CAT_ANALYTICS},
    {"careers.hotjar.com", CAT_ANALYTICS},
    {"events.hotjar.io", CAT_ANALYTICS},
    {"mouseflow.com", CAT_ANALYTICS},
    {"cdn.mouseflow.com", CAT_ANALYTICS},
    {"o2.mouseflow.com", CAT_ANALYTICS},
    {"gtm.mouseflow.com", CAT_ANALYTICS},
    {"api.mouseflow.com", CAT_ANALYTICS},
    {"tools.mouseflow.com", CAT_ANALYTICS},
    {"cdn-test.mouseflow.com", CAT_ANALYTICS},
    {"freshmarketer.com", CAT_ANALYTICS},
    {"claritybt.freshmarketer.com", CAT_ANALYTICS},
    {"fwtracks.freshmarketer.com", CAT_ANALYTICS},
    {"luckyorange.com", CAT_ANALYTICS},
    {"api.luckyorange.com", CAT_ANALYTICS},
    {"realtime.luckyorange.com", CAT_ANALYTICS},
    {"cdn.luckyorange.com", CAT_ANALYTICS},
    {"w1.luckyorange.com", CAT_ANALYTICS},
    {"upload.luckyorange.net", CAT_ANALYTICS},
    {"cs.luckyorange.net", CAT_ANALYTICS},
    {"settings.luckyorange.net", CAT_ANALYTICS},
    {"stats.wp.com", CAT_ANALYTICS},

    // ===== Error Trackers =====
    {"notify.bugsnag.com", CAT_ERROR_TRACKERS},
    {"sessions.bugsnag.com", CAT_ERROR_TRACKERS},
    {"api.bugsnag.com", CAT_ERROR_TRACKERS},
    {"app.bugsnag.com", CAT_ERROR_TRACKERS},
    {"browser.sentry-cdn.com", CAT_ERROR_TRACKERS},
    {"app.getsentry.com", CAT_ERROR_TRACKERS},

    // ===== Social Trackers =====
    {"pixel.facebook.com", CAT_SOCIAL},
    {"an.facebook.com", CAT_SOCIAL},
    {"static.ads-twitter.com", CAT_SOCIAL},
    {"ads-api.twitter.com", CAT_SOCIAL},
    {"ads.linkedin.com", CAT_SOCIAL},
    {"analytics.pointdrive.linkedin.com", CAT_SOCIAL},
    {"ads.pinterest.com", CAT_SOCIAL},
    {"log.pinterest.com", CAT_SOCIAL},
    {"trk.pinterest.com", CAT_SOCIAL},
    {"events.reddit.com", CAT_SOCIAL},
    {"events.redditmedia.com", CAT_SOCIAL},
    {"ads.youtube.com", CAT_SOCIAL},
    {"ads-api.tiktok.com", CAT_SOCIAL},
    {"analytics.tiktok.com", CAT_SOCIAL},
    {"ads-sg.tiktok.com", CAT_SOCIAL},
    {"analytics-sg.tiktok.com", CAT_SOCIAL},
    {"business-api.tiktok.com", CAT_SOCIAL},
    {"ads.tiktok.com", CAT_SOCIAL},
    {"log.byteoversea.com", CAT_SOCIAL},

    // ===== Mix (Yahoo, Yandex, Unity) =====
    {"ads.yahoo.com", CAT_MIX},
    {"analytics.yahoo.com", CAT_MIX},
    {"geo.yahoo.com", CAT_MIX},
    {"udcm.yahoo.com", CAT_MIX},
    {"analytics.query.yahoo.com", CAT_MIX},
    {"partnerads.ysm.yahoo.com", CAT_MIX},
    {"log.fc.yahoo.com", CAT_MIX},
    {"gemini.yahoo.com", CAT_MIX},
    {"adtech.yahooinc.com", CAT_MIX},
    {"extmaps-api.yandex.net", CAT_MIX},
    {"appmetrica.yandex.ru", CAT_MIX},
    {"adfstat.yandex.ru", CAT_MIX},
    {"metrika.yandex.ru", CAT_MIX},
    {"offerwall.yandex.net", CAT_MIX},
    {"adfox.yandex.ru", CAT_MIX},
    {"auction.unityads.unity3d.com", CAT_MIX},
    {"webview.unityads.unity3d.com", CAT_MIX},
    {"config.unityads.unity3d.com", CAT_MIX},
    {"adserver.unityads.unity3d.com", CAT_MIX},

    // ===== OEMs =====
    {"iot-eu-logser.realme.com", CAT_OEM},
    {"iot-logser.realme.com", CAT_OEM},
    {"bdapi-ads.realmemobile.com", CAT_OEM},
    {"bdapi-in-ads.realmemobile.com", CAT_OEM},
    {"api.ad.xiaomi.com", CAT_OEM},
    {"data.mistat.xiaomi.com", CAT_OEM},
    {"data.mistat.india.xiaomi.com", CAT_OEM},
    {"data.mistat.rus.xiaomi.com", CAT_OEM},
    {"sdkconfig.ad.xiaomi.com", CAT_OEM},
    {"sdkconfig.ad.intl.xiaomi.com", CAT_OEM},
    {"tracking.rus.miui.com", CAT_OEM},
    {"adsfs.oppomobile.com", CAT_OEM},
    {"adx.ads.oppomobile.com", CAT_OEM},
    {"ck.ads.oppomobile.com", CAT_OEM},
    {"data.ads.oppomobile.com", CAT_OEM},
    {"metrics.data.hicloud.com", CAT_OEM},
    {"metrics2.data.hicloud.com", CAT_OEM},
    {"grs.hicloud.com", CAT_OEM},
    {"logservice.hicloud.com", CAT_OEM},
    {"logservice1.hicloud.com", CAT_OEM},
    {"logbak.hicloud.com", CAT_OEM},
    {"click.oneplus.cn", CAT_OEM},
    {"open.oneplus.net", CAT_OEM},
    {"samsungads.com", CAT_OEM},
    {"smetrics.samsung.com", CAT_OEM},
    {"nmetrics.samsung.com", CAT_OEM},
    {"samsung-com.112.2o7.net", CAT_OEM},
    {"analytics-api.samsunghealthcn.com", CAT_OEM},
    {"iadsdk.apple.com", CAT_OEM},
    {"metrics.icloud.com", CAT_OEM},
    {"metrics.mzstatic.com", CAT_OEM},
    {"api-adservices.apple.com", CAT_OEM},
    {"books-analytics-events.apple.com", CAT_OEM},
    {"weather-analytics-events.apple.com", CAT_OEM},
    {"notes-analytics-events.apple.com", CAT_OEM},
};

#define TEST_DOMAINS_COUNT (sizeof(TEST_DOMAINS) / sizeof(TEST_DOMAINS[0]))

// Category names for display
static const char* getCategoryName(TestCategory cat) {
    switch (cat) {
        case CAT_ADS:            return "ads";
        case CAT_ANALYTICS:      return "analytics";
        case CAT_ERROR_TRACKERS: return "error_trackers";
        case CAT_SOCIAL:         return "social";
        case CAT_MIX:            return "mix";
        case CAT_OEM:            return "oem";
        default:                 return "unknown";
    }
}

#endif // TEST_DOMAINS_H
