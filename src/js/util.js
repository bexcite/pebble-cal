/* global constants */

var util = (function(constants){

var util = {};


util.getStoredUserInfo = function() {
    var db = window.localStorage;
    var userInfoString = db.getItem("userInfo");
    if (userInfoString) {
        console.log("getStoredUserInfo = " + userInfoString);
        var userInfo = JSON.parse(userInfoString);
        return userInfo;
    }
    console.log("getStoredUserInfo = null");
    return null;
};


util.saveUserInfo = function(userInfo) {
    var db = window.localStorage;
    db.setItem("userInfo", JSON.stringify(userInfo));
};


/*
 * Returns the right datetime string for timeMin
 */
util.get_current_utc_time = function () {
    var now = new Date();
    return util.convert_date_to_utc_string(now);
};


/*
 * Construct the right datetime format YYYY:MM:DDTHH:MM:SS.MMMZ
 */
util.convert_date_to_utc_string = function(d) {
    var now = d;

    var number = function(d) {
        if (d < 10) { return "0" + d; }
        return d + "";
    };

    var number3 = function(d) {
        if (d < 10) { return "00" + d; }
        if (d < 100) { return "0" + d; }
        return d + "";
    };

    var year = number(now.getUTCFullYear());
    var month = number(now.getUTCMonth() + 1);
    var day = number(now.getUTCDate());
    var hour = number(now.getUTCHours());
    var minutes = number(now.getUTCMinutes());
    var seconds = number(now.getUTCSeconds());
    var milliseconds = number3(now.getUTCMilliseconds());

    var result = year +
        "-" + month +
        "-" + day +
        "T" + hour +
        ":" + minutes +
        ":" + seconds +
        "." + milliseconds +
        "Z";

    return result;

};

/*
 * Short the summary string and append ... (horizaontal ellipsis)
 */
util.shortenSummary = function(summary) {
    if (!summary) {
      return "";
    }
    if (summary.length < constants.SUMMARY_LENGTH) {
        return summary;
    } else {
        return summary.substring(0, constants.SUMMARY_LENGTH) + String.fromCharCode(0x2026); // added ... char to the end
    }
};

/*
 * Object to Console help method
 */
util.otc = function(obj, name) {
    console.log("===== Object : " + name + " === ");
    for (var key in obj) {
        console.log(name + "['" + key + "'] = " + obj[key]);
    }
};

/*
 * Object to String help method
 */
util.otc = function(obj, name) {
    console.log("===== Object : " + name + " === ");
    for (var key in obj) {
        console.log(name + "['" + key + "'] = " + obj[key]);
    }
};


var testEventNow;
util.getTestEvents = function() {
    if (!testEventNow) {
        testEventNow = (new Date()).getTime();
    }
    var now = testEventNow;
    var firtsEventStart = 10 * 60 * 1000;
    var eventTime = 20 * 60 * 1000;
    var eventsDelta = 45 * 60 * 1000;
    var events = [

    {
        type: constants.EVENT_TYPE_DEFAULT,
        start: (new Date(now + 1*firtsEventStart + 0*eventTime)).getTime(),
        end: (new Date(now + 1*firtsEventStart + 1*eventTime)).getTime(),
        summary: "First Event"
    },

    {
        type: constants.EVENT_TYPE_DEFAULT,
        start: (new Date(now + 1*firtsEventStart + 1*eventTime + eventsDelta)).getTime(),
        end: (new Date(now + 1*firtsEventStart + 2*eventTime + eventsDelta)).getTime(),
        summary: "Second Event"
    },

    {
        type: constants.EVENT_TYPE_DEFAULT,
        start: (new Date(now + 1*firtsEventStart + 2*eventTime + 2*eventsDelta)).getTime(),
        end: (new Date(now + 1*firtsEventStart + 3*eventTime + 2*eventsDelta)).getTime(),
        summary: "Third Event"
    }


    ];

    return events;

};




return util;

})(constants);
