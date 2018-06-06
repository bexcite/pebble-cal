/* global util, constants, MessageQueue */

(function(Pebble, util, constants, MessageQueue){


var messageQueue = new MessageQueue();

/* RETRY
// Variable to count number of retries
var retryCount = 0;
// Variable to store setTimeout obj.
var retryTimeoutObj;
*/


/*
 * Process ready from the watch
 */
Pebble.addEventListener("ready",
		function(e) {
	console.log("ready !!! mes");
    // TODO: need to remove primary???
    //window.localStorage.removeItem("primaryCalendar");

    fetch_and_send_events();

});


/*
 * Show the config/display page
 */
Pebble.addEventListener("showConfiguration",
		function(e) {

    console.log("Show configuraiton Yay!!!!");
    //console.log("config_url=" + CONFIG_URL);



    var url = constants.CONFIG_URL + "?" + // + "code=" + "code12345"
        "client_id=" + encodeURIComponent(constants.CONFIG_GOOGLE_CLIENT_ID) +
        "&redirect_uri=" + encodeURIComponent(constants.CONFIG_GOOGLE_REDIRECT_URI) +
        "&scope=" + encodeURIComponent(constants.CONFIG_GOOGLE_SCOPE);

    // Add Stored User Info
    var storedUserInfo = util.getStoredUserInfo();
    console.log('storedUserInfo = ' + storedUserInfo);
    if (storedUserInfo) {
        console.log('Add storedUserInfo = ' + storedUserInfo);
        url += "&sui=" + encodeURIComponent(JSON.stringify(storedUserInfo));
    }

    var db = window.localStorage;
    var code = db.getItem("code");
    if (code) {
        url += "&cp=true";
    }


    console.log("config_url = " + url);

    // Goto configuration.html file on AWS EC2/Stanfy server
    Pebble.openURL(url);

});


/*
 * Receives the code back from configuration window.
 */
Pebble.addEventListener("webviewclosed", function (e) {
    console.log("webviewclosed CALL!");
    var json = e.response;
    //console.log("e.response = " + e.response);
    util.otc(e, "e");
    if (!e.response || e.response.length === 0) {
        console.log("It was close button.");
        return;
    }

    // TODO: Error handling on parse error.
    var config;
    try {
        config = JSON.parse(decodeURIComponent(json));
    } catch (error_json) {
        console.log("CLOSE WEBVIEW - 1 " + error_json);
        return;
    }

    var db = window.localStorage;

    if (config.error || config.unlink) {
        console.log("config.error = " + config.error);
        console.log("config.unlink = " + config.unlink);
        console.log("Error - remove all tokens");
        db.removeItem("code");
        db.removeItem("access_token");
        db.removeItem("refresh_token");
        db.removeItem("primaryCalendar");
        db.removeItem("selectedCalendars"); // TODO: Currently we do not store selected calendars
        // Send Error to the Pebble
        sendErrorToPebble();
        return;
    }

    // Process 'code' param
    if (config.code) {
        var code = config.code;
        console.log("config.code returned = " + code);

        var old_code = db.getItem("code");
        if (old_code != code) {
            console.log("Code is the newest one. Rewrite it ...");
            db.setItem("code", code);
            db.removeItem("access_token");
            db.removeItem("refresh_token");
            db.removeItem("primaryCalendar");
            db.removeItem("selectedCalendars"); // TODO: Currently we do not store selected calendars
        }

        // Process 'allSelectedCalendars' param
        var fetchAllSelectedCalendars = config.fetchAllSelectedCalendars;
        var userInfo = {};
        userInfo.fetchAllSelectedCalendars = fetchAllSelectedCalendars === true ? true : false;
        util.otc(userInfo, "savedUserInfo");
        util.saveUserInfo(userInfo);

        resolve_tokens(code, function() {
            //console.log("!!!!! DO SMTH HERE!!!!");
            fetch_and_send_events();
        });

        return;

    }

    console.log("Configuration Save Action!");



});



/*
 * Receives message from Pebble.
 * On any message
 */
Pebble.addEventListener("appmessage",
    function(e) {

        //console.log("Received message \"cmdUpdate\": " + e.payload["cmdUpdate"]);
        //

        // Chech for cmdUpdate command
        //console.log("Check for cmdUpdate: " + e.payload["cmdUpdate"]);
        if (e.payload.cmdUpdate) {
            console.log("YES it is cmdUpdate, so update events ...");

            // Show the key we received from Pebble
            util.otc(e.payload, "e.payload");

            // Fetch Events from Google Cal and send events to Pebble
            fetch_and_send_events();
        }

    }
);



/*
 * Fetches events from Google calendar and sends it to Pebble.
 */
function fetch_and_send_events(useAllSelected) {

    // TODO: Return to default useAllSelected = false;
    useAllSelected = typeof useAllSelected !== 'undefined' ? useAllSelected: false;

    var userInfo = util.getStoredUserInfo();
    if (userInfo) {
        useAllSelected = userInfo.fetchAllSelectedCalendars;
    }

    console.log("FETCH and SEND Events, useAllSelected = " + useAllSelected);

/*
    // TEST Events without any login
    if (constants.TEST_EVENTS) {
        console.log("Sending TEST_EVENTS");
        sendEventsToPebble(util.getTestEvents());
        //sendEventsToPebble([]);
        return;
    }
*/
    //sendEventsToPebbleWithRetry(events);

    if (!useAllSelected) {
        // Use only primaryCalendar
        console.log("Using only primaryCalendar");

        use_primary_calendar(function(primaryCalendar) {

            console.log("Primary calendar is ready id=" + primaryCalendar.id +
                    ", summary=" + primaryCalendar.summary);

            fetch_events(primaryCalendar, function(events) {
                console.log("Events is ready " + events.length);

                // Debug show
                for (var i = 0; i < events.length; i++) {
                    var item = events[i];
                    console.log(i + ". event summary=" + item.summary +
                        ", start=" + item.start +
                        ", end=" + item.end +
                        ", id=" + item.id);
                    //util.otc(item.start, "item.start");
                }

                // RETRY
                //sendEventsToPebbleWithRetry(events);
                sendEventsToPebble(events);

            });

        });

    } else {
        console.log("Using allSelectedCalendar!");

        use_selected_calendars(function(selectedCalendars) {
            //console.log("We have selectedCalendars = " + selectedCalendars);

            // create array with completion Flags
            var completionFlags = {};
            var sortedEvents = [];

            for (var i = 0; i < selectedCalendars.length; i++) {
                var currentCalendar = selectedCalendars[i];

                // create completionFlag == false
                completionFlags[currentCalendar.id] = false;

                // fetch events
                console.log("Fetch events for calendar["+ i +"] = " + currentCalendar.summary);
// Here we can make a function %)
/* jshint -W083 */
                fetch_events(currentCalendar, function(events, calendar) {
                    console.log("Received events for calendar = " + calendar.summary);

                    // add all evetns to sorted array
                    addEventsToSortedArray(sortedEvents, events);

                    // set completionFlag = true
                    completionFlags[calendar.id] = true;

                    // check if all completionFlags == true
                    var allCompleted = true;
                    for (var calendarKey in completionFlags) {
                        allCompleted = allCompleted && completionFlags[calendarKey];
                    }

                    // then send result to Pebble
                    if (allCompleted) {
                        console.log("ALL Completed - send result to Pebble");
                        for (var j = 0; j < sortedEvents.length; j++) {
                            console.log("sortedEvents[" + j + "].start = " + sortedEvents[j].start);
                            console.log("sortedEvents[" + j + "].id = " + sortedEvents[j].id);
                            console.log("sortedEvents[" + j + "].summary = " + sortedEvents[j].summary);
                        }

                        // Send events to Pebble
                        // RETRY
                        //sendEventsToPebbleWithRetry(sortedEvents);
                        sendEventsToPebble(sortedEvents);

                    } else {
                        console.log("NOT ALL Completed yet - next time");
                    }


                });
/* jshint +W083 */

            }

        });


    }

}

function addEventsToSortedArray(sortedEvents, events) {
    for (var i = 0; i < events.length; i++) {
        addEventToSortedArray(sortedEvents, events[i]);
    }
}

function addEventToSortedArray(sortedEvents, event) {
    for (var i = 0; i < sortedEvents.length; i++) {
        var currentEvent = sortedEvents[i];
        if (currentEvent.start > event.start) {
            sortedEvents.splice(i, 0, event);
            return;
        }
    }
    sortedEvents.push(event);
}

/*
 *  Send sendObj to Pebble with retry.
 */
function sendEventsToPebble(events) {
    console.log("Send events to Pebble ...");

    var sendObj = prepareSendObj(events);

    messageQueue.send(sendObj);

/*

    var transactionId = Pebble.sendAppMessage(
        sendObj,
        function (e) {
            // in send success case
            console.log("Successfull message with transactionId = " + e.data.transactionId);
            //retryCount = 0;
            resetRetrySend();

            // Test
            //Pebble.showSimpleNotificationOnPebble("Test", "Transaction SUCCESS " + my2);
        },
        function (e) {
            // in send error case
            console.log("Unable to deliver message with transactionId = " +
                e.data.transactionId);
            console.log("!!!!!!----!!!!!! Error is " + e);
            util.otc(e, "e");
            util.otc(e.data, "otc.data");
            util.otc(e.data.error, "otc.data.error");
            util.otc(e.error, "e.error");
            console.log("Will retry sending in some time");

            // Go to next retry attempt
            retrySendEvents(sendObj);

        }
    );
*/

}

/*
 *  Send received events to Pebble with retry
 */
 /* RETRY
function sendEventsToPebbleWithRetry(events) {
    console.log("Send events to Pebble with retry");
    // Prepare sendObj from events
    var sendObj = prepareSendObj(events);
    util.otc(sendObj, "sendObj");

    // Reset retry send if any
    resetRetrySend();

    // Send events to Pebble with retry attempts
    sendEventsToPebble(sendObj);
}
*/

/*
 * Reset retry counter and timeOutObject.
 * Need if MAX_ATTEMPTS reached OR new events data received and will try send a new portion.
 */
 /* RETRY
function resetRetrySend() {
    console.log("== Reset retry send");
    retryCount = 0;
    clearTimeout(retryTimeoutObj);
}
*/

/*
 *  Will try to send sendObj to Pebble until MAX_ATTEMPTS will be reached
 */
 /* RETRY
function retrySendEvents(sendObj) {

    retryCount++;
    console.log("retryCount = " + retryCount);

    // If reach MAX_ATTEMPTS - stop trying
    if (retryCount > constants.RETRY_SEND_MAX_ATTEMPTS) {
        resetRetrySend();
        console.log("=== STOP TRYING to SEND THIS MESSAGE ===== ");
        return;
    }

    var retryTimeout = retryCount * constants.RETRY_SEND_TIMEOUT;
    console.log("retryTimeout = " + retryTimeout);


    // Retry in 5 sec for 5 time.
//    retryTimeoutObj = setTimeout(function() {
//        console.log("======== Retry Send Events to Pebble ==============");
//        util.otc(sendObj, "sendObj");
//        sendEventsToPebble(sendObj);
//    }, retryTimeout);

    console.log("======== Retry Send Events to Pebble ==============");
    util.otc(sendObj, "sendObj");
    sendEventsToPebble(sendObj);

}
*/

/*
 * Send Error to Pebble
 */
function sendErrorToPebble() {

    // TEST Events without any login
    if (constants.TEST_EVENTS) {
        console.log("Sending TEST_EVENTS");
/*
        var testEvents = util.getTestEvents();

        var i;
        for (i = 0; i < testEvents.length; i++) {
            util.otc(testEvents[i], "testEvents["+i+"]");
        }

        var testEventsPrepared = prepareSendObj(testEvents);


        for (i = 0; i < testEventsPrepared.length; i++) {
            util.otc(testEventsPrepared[i], "testEventsPrepared["+i+"]");
        }
*/

        sendEventsToPebble(util.getTestEvents());
        //sendEventsToPebble([]);
        return;
    }


    console.log("Send Error to Pebble");
    // Prepare error obj
    var sendObj = {};
/*jshint -W069 */
    sendObj["cmdError"] = 1; // 1- Auth ERROR
/*jshint +W069 */

    // RETRY
    //resetRetrySend();

    //sendEventsToPebble(sendObj);
    messageQueue.send(sendObj);
}


function prepareSendObj(events) {

    //timezone offset
    var timezoneOffset = (new Date()).getTimezoneOffset() * 60; //s
    //var timezoneOffset = 0; // All Conversion to localtime happenning on Pebble. (Dec 13, 2015 -- Pavlo)

    // Prepare object to send
    //var sendObj = {"1": "", "2": "", "3": ""};
    var sendObj = {};
    var isCurrentDay = true;
    var today = new Date();
    var todayStr = today.getYear() + " " + today.getMonth() + today.getDate();

    var index = 0;
    var objSize = 0;
    var maxObjectCount = 3;
    var indexId = 0;
    for (var i = 0; i < events.length && indexId < maxObjectCount; i++) {
        //console.log("prepare i="+i);
        var item = events[i];

        //check dates
        var eventDate = new Date(item.start);
        var eventDateStr =  eventDate.getYear() + " " + eventDate.getMonth() + eventDate.getDate();

        //console.log("todayDate = " + todayStr + ", eventDate = " + eventDateStr);

        // NOTE: Don't ask we here is !(a == b) instead of (a != b)
        // it is because it works only in first case, a and b are strings.
        // TBD: Fix this mess with !(==)
/* jshint -W018 */
        if (constants.DAY_SPLITTER === true) {

            if (isCurrentDay && !(todayStr == eventDateStr)) {
                isCurrentDay = false;

                //populate object data

                index = constants.EVENT_STRUCT_START + indexId * constants.EVENT_STRUCT_SIZE; // Number of event
                sendObj[index + 0] = constants.EVENT_TYPE_EMPTY; // Type
                sendObj[index + 1] = 0; // Start millis to seconds
                sendObj[index + 2] = 0; // End millis to seconds
                sendObj[index + 3] = "";
                objSize += 4 + 4 + 4 + 0 + 4*7;

                //FIX bug with skipping event
                //i++;
                indexId++;
                if (indexId >= maxObjectCount) {
                    break;
                }
            }

        }
/* jshint +W018 */



        //populate object data
        //item = events[i];
        index = constants.EVENT_STRUCT_START + indexId * constants.EVENT_STRUCT_SIZE; // Number of event
        sendObj[index + 0] = item.type; //EVENT_TYPE_DEFAULT; // Type
        //console.log("type - done");
        sendObj[index + 1] = item.start / 1000 - timezoneOffset; // Start millis to seconds
        //console.log("start - done");
        sendObj[index + 2] = item.end / 1000 - timezoneOffset; // End millis to seconds
        //console.log("end - done");
        sendObj[index + 3] = util.shortenSummary(item.summary); // cut here to 25 characters + add ... char to the end
        //console.log("summary - done");
        objSize += 4 + 4 + 4 + sendObj[index + 3].length + 4*7;

        indexId++;
    }

    //console.log("objSize calculating");

    console.log("objSize = " + objSize/* + String.fromCharCode(0x2026)*/);

    return sendObj;
}


/*
 * Fetches events from Google calendar and return control to act.
 */
function fetch_events(primaryCalendar, act) {
    use_access_token(function(access_token) {
        console.log("Get eventsList ... for primaryCalendar.id = " + primaryCalendar.id);
        var timeMin = util.get_current_utc_time();
        console.log("timeMin = " + timeMin);
        var req = new XMLHttpRequest();
        req.open("GET", "https://www.googleapis.com/calendar/v3/calendars/" + encodeURIComponent(primaryCalendar.id) +
            "/events?fields=" + encodeURIComponent("items(id,summary,start,end)") + //items(id,summary,start,end) +
            "&maxResults=5" +
            "&singleEvents=True" +
            "&orderBy=startTime" +
            "&timeMin=" + encodeURIComponent(timeMin), true);
        req.onload = function(e) {
            if (req.readyState == 4) {
                if (req.status == 200) {
                    console.log("Received Event List ====== "); // + req.responseText
                    var eventList = JSON.parse(req.responseText);
                    //console.log("response = " + eventList.items[0].summary);
                    if (eventList.items) {


                        console.log("Event list go to next");

                        if (act) {

                            console.log("convert to shortItem");
                            var shortEvents = [];
                            for (var i = 0; i < eventList.items.length; i++) {
                                var item = eventList.items[i];
                                //util.otc(item, "item long");
                                //util.otc(item.start, "item long start");
                                //util.otc(item.end, "item long end");
																//util.otc(item.summary, "item long end");
                                var shortItem = {};
                                shortItem.id = item.id;
                                shortItem.summary = item.summary;

																// This was a bug on Pebble Round
																// with undefined summary event. [Pavlo on 2016.01.24]
																if (!shortItem.summary) {
																	console.log('Skip event because item.summary is undefined.');
																	continue;
																}

                                if (item.start.date && item.end.date) {
                                    // All-day event
                                    console.log("Should skip event " + item.summary);
                                    continue;
                                    // shortItem.type = constants.EVENT_TYPE_ALL_DAY;
                                    // shortItem.start = Date.parse(item.start.date.replace(/-/gi,"/")); //string.replace(searchvalue,newvalue)
                                    // shortItem.end = Date.parse(item.end.date.replace(/-/gi,"/"));
                                } else {
                                    // Timed event
                                    shortItem.type = constants.EVENT_TYPE_DEFAULT;
                                    shortItem.start = Date.parse(item.start.dateTime);
                                    shortItem.end = Date.parse(item.end.dateTime);
                                }

                                //console.log("shortItem.start = " + new Date(shortItem.start)/* + ", " + util.convert_date_to_utc_string(shortItem.start) */);
                                //console.log("shortItem.end = " + new Date(shortItem.end)/* + ", " + util.convert_date_to_utc_string(shortItem.end)*/);

                                shortEvents.push(shortItem);
                            }

                            //Call next function with primary calendar short version id/summary
                            act(shortEvents, primaryCalendar);

                        }




                    }
                } else {
                    console.log("Error in request get events " + req.responseText);
                }
            }
        };
        req.setRequestHeader("Authorization", "Bearer " + access_token);
        req.send();
    });

}


/*
 * Use gate for primaryCalendar object
 * - act is a function that accepts primaryCalendar{id, summary} as a parameter
 */
function use_primary_calendar(act) {
    console.log("Use primaryCalendar ..");
    var db = window.localStorage;
    var pCalString = db.getItem("primaryCalendar");
    if (!pCalString) {
        console.log("Fetch primaryCalendar ...");
        fetch_primary_calendar(act);
    } else {
        console.log("Use primaryCalendar from db ...");
        var primaryCalendar = JSON.parse(pCalString);
        act(primaryCalendar);
    }
}

/*
 * Uses selectedCalendars from storage if present
 * - act is a function that accepts an array of calendars [{id, summary, primary}]
 */
function use_selected_calendars(act) {
    console.log("Use selectedCalendars ..");
    var db = window.localStorage;
    var pSelCalString = db.getItem("selectedCalendars"); // TODO: Currently we do not store selected calendars
    // so it will be always empty
    if (!pSelCalString) {
        console.log("Fetch selectedCalendars ...");
        fetch_selected_calendars(act);
    } else {
        console.log("Use selectedCalendars from db ...");
        var selectedCalendars = JSON.parse(pSelCalString);
        act(selectedCalendars);
    }
}

/*
 * Fetched all 'selected' calendars
 * - next is a function that accepts an array of calendars [{id, summary, primary}]
 */
function fetch_selected_calendars(next) {
    use_access_token(function(access_token) {
        console.log("Get selected calendarList ...");
        var req = new XMLHttpRequest();
        req.open("GET", "https://www.googleapis.com/calendar/v3/users/me/calendarList?fields=" + encodeURIComponent("items(id,summary,primary,selected)") +
            "", true);
        req.setRequestHeader('Authorization', 'Bearer ' + access_token);

        req.onload = function(e) {
            if (req.readyState == 4) {
                if (req.status == 200) {
                    console.log("Received Calendar List (for selected) ====== ");
                    var calendarList = JSON.parse(req.responseText);

                    if (calendarList.items) {

                        // Find selected calendars and primary
                        var selectedCalendars = [];
                        for (var i = 0; i <  calendarList.items.length; i++) {
                            var item = calendarList.items[i];
                            // util.otc(item, "item[" + i + "]");
                            //console.log("item id=" + item.id + ",summary=" + item.summary);
                            if ((item.primary && item.primary === true) || item.selected === true)  {
                                //console.log("Primary - " + item.summary);
                                var shortItem = {
                                    "id" : item.id,
                                    "summary" : item.summary,
                                    "primary" : (item.primary && item.primary === true) ? true : false
                                };
                                selectedCalendars.push(shortItem);
                                //util.otc(shortItem, "selectedCalendars[" + i + "]");

                                //primaryCalendar = item;
                                //break;
                            }
                        }

                        if (!selectedCalendars.length) {
                            console.log("Error selected or primaryCalendar not found!");
                            return;
                        }

                        //console.log("selctedCalendars = " + JSON.stringify(selectedCalendars));

                        // TODO: Think should we store selectedCalendats in cache???
                        //var db = window.localStorage;
                        //db.setItem("selectedCalendars", JSON.stringify(selectedCalendars));

                        if (next) {
                            //Call next function with primary calendar short version id/summary
                            next(selectedCalendars);
                        }

                        //util.otc(selectedItems, "selectedItems");
                    }

                } else {
                    console.log("Error in request get selected calendarList " + req.responseText);
                }
            }
        };

        req.send();
    });
}


/*
 * Fetches calendarList and returns the primary calendar.
 * - next is a function that accepts primaryCalendar{id, summary} as a parameter.
 */
function fetch_primary_calendar(next) {
    use_access_token(function(access_token) {
        console.log("Get calendarList ...");
        var req = new XMLHttpRequest();
        req.open("GET", "https://www.googleapis.com/calendar/v3/users/me/calendarList?fields=" + encodeURIComponent("items(id,summary,primary,selected)") +
            "", true);
        req.onload = function(e) {
            if (req.readyState == 4) {
                if (req.status == 200) {
                    console.log("Received Calendar List ====== ");
                    //console.log(req.responseText);
                    var calendarList = JSON.parse(req.responseText);
                    console.log("response = " + calendarList.items[0].summary);
                    if (calendarList.items) {

                        // Find primary calendar
                        var primaryCalendar;
                        var selectedCalendars = [];
                        for (var i = 0; i <  calendarList.items.length; i++) {
                            var item = calendarList.items[i];
                            //util.otc(item, "item[" + i + "]");
                            //console.log("item id=" + item.id + ",summary=" + item.summary);
                            if (item.primary && item.primary === true) {
                                console.log("Primary - " + item.summary);
                                primaryCalendar = item;
                                //break;
                            }
                            if (item.selected === true) {
                                selectedCalendars.push(item);
                                //util.otc(item, "selectedCalendars[" + i + "]");
                            }

                        }

                        if (!primaryCalendar) {
                            console.log("Error primaryCalendar not found!");
                            return;
                        }

                        console.log("Primary here - " + primaryCalendar.summary);

                        var calShort = {
                            "id": primaryCalendar.id,
                            "summary": primaryCalendar.summary
                        };

                        var db = window.localStorage;
                        db.setItem("primaryCalendar", JSON.stringify(calShort));

                        if (next) {
                            //Call next function with primary calendar short version id/summary
                            next(calShort);
                        }

                        //util.otc(selectedItems, "selectedItems");


                    }
                } else {
                    console.log("Error in request get calendarList " + req.responseText);
                }
            }
        };
        req.setRequestHeader('Authorization', 'Bearer ' + access_token);
        req.send();
    });
}


/*
 * Resolves tokens by given code
 */
function resolve_tokens(code, next) {
    console.log("Resolve tokens ...");
    var req = new XMLHttpRequest();
    req.open("POST", "https://accounts.google.com/o/oauth2/token", true);
    req.setRequestHeader("Content-type", "application/x-www-form-urlencoded");

    req.onload = function(e) {
        var db = window.localStorage;
        if (req.readyState == 4) {
            if (req.status == 200) {
                console.log("responseText = " + req.responseText);
                var response = JSON.parse(req.responseText);
                if (response.refresh_token && response.access_token) {
                    console.log("set to db token = " + response.refresh_token + ", access_token = " + response.access_token);
                    db.setItem("refresh_token", response.refresh_token);
                    db.setItem("access_token", response.access_token);


                    // Call next function
                    if (next) {
                        next();
                    }

                    return;
                }
            } else {
                console.log("Resolve tokens ERROR!!!! " + req.responseText);
            }
        }
    };

    var resolve_params = "code=" + encodeURIComponent(code) +
        "&client_id=" + encodeURIComponent(constants.CONFIG_GOOGLE_CLIENT_ID) +
        "&client_secret=" + encodeURIComponent(constants.CONFIG_GOOGLE_CLIENT_SECRET) +
        "&redirect_uri=" + encodeURIComponent(constants.CONFIG_GOOGLE_REDIRECT_URI) +
        "&grant_type=authorization_code";


    req.send(resolve_params);

    console.log("resolve_tokens SENT");
}


/*
 * Function that uses access token. Code - is a function.
 */
function use_access_token(code) {
    console.log("Use access token ...");
    var db = window.localStorage;
    var refresh_token = db.getItem("refresh_token");
    var access_token = db.getItem("access_token");

    console.log("refresh_token=" + refresh_token +
        ", access_token=" + access_token);

    //if (!refresh_token) return;

    valid_token(access_token, code, function() {
        refresh_access_token(refresh_token, code);
    });
}


/*
 * Validates the access token.
 * - access_token - the access_token to validate
 * - good - the code to run when the access_token is good, run like good(access_token)
 * - bad - the code to run when the access_token is expired, run like bad()
 */
function valid_token(access_token, good, bad) {
    console.log("Validate the access_token ... ");

    if (!access_token) {
        console.log("access_token is empty");
        bad();
        return;
    }

    var req = new XMLHttpRequest();
    req.open("GET", "https://www.googleapis.com/oauth2/v1/tokeninfo?access_token=" + encodeURIComponent(access_token), true);
    req.onload = function(e) {
        if (req.readyState == 4) {
            if (req.status == 200) {
                var result = JSON.parse(req.responseText);

                if (result.audience != constants.CONFIG_GOOGLE_CLIENT_ID) {
                    var db = window.localStorage;
                    db.removeItem("code");
                    db.removeItem("access_token");
                    db.removeItem("refresh_token");
                    //db.setItem("code_error", "There was an error validating your Google Authentication. Please re-authorize access to your account.");
                    return;
                }
                console.log("access_token is valid continue");
                good(access_token);

            } else {

                bad();

            }
        }

        //bad();
    };
    req.send(null);
}


/*
 * Refresh a stale access_token.
 * - refresh_token - the refresh_token to use to retreive a new access_token
 * - code - code to run with the new access_token, run like code(access_token)
 */
function refresh_access_token(refresh_token, code) {
    console.log("Refresh access_token ...");

    if (!refresh_token) {
        console.log("refresh_token is empty");
        sendErrorToPebble();
        return;
    }

    var req = new XMLHttpRequest();
    req.open("POST", "https://accounts.google.com/o/oauth2/token", true);
    req.setRequestHeader("Content-type", "application/x-www-form-urlencoded");

    req.onload = function(e) {
        if (req.readyState == 4) {
            var result;
            var db;
            if (req.status == 200) {
                console.log("refresh status 200 ");
                result = JSON.parse(req.responseText);

                if (result.access_token) {
                    db = window.localStorage;
                    db.setItem("access_token", result.access_token);
                    code(result.access_token);
                }
            } else {
                console.log("Refresh Error! " + req.status + " " + req.responseText);
                result = JSON.parse(req.responseText);
                if (result.error && result.error == "invalid_grant") {
                    // clean all
                    console.log("Invalid grant - clean all tokens");
                    db = window.localStorage;
                    db.removeItem("code");
                    db.removeItem("access_token");
                    db.removeItem("refresh_token");
                }
                sendErrorToPebble();
            }
        }
    };

    req.send("refresh_token=" + encodeURIComponent(refresh_token) +
        "&client_id=" + encodeURIComponent(constants.CONFIG_GOOGLE_CLIENT_ID) +
        "&client_secret=" + encodeURIComponent(constants.CONFIG_GOOGLE_CLIENT_SECRET) +
        "&grant_type=refresh_token");

}

})(Pebble, util, constants, MessageQueue);
