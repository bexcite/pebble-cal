// Initialize Global Var
var DEBUG = 0;

function createFunctionWithTimeout(callback, opt_timeout) {
  var called = false;

  var nf = function() {
    if (!called) {
      called = true;
      callback();
    }
  }
  setTimeout(nf, opt_timeout || 1000);
  return nf;
}

function getGoogleLoginUrl() {
    var conf = {}; //JSON.parse(json);

    //alert('return_to=' + getQueryVariable('return_to'))

    conf.GOOGLE_CLIENT_ID = decodeURIComponent(getQueryVariable("client_id"));
    conf.REDIRECT_URI = decodeURIComponent(getQueryVariable("redirect_uri"));
    conf.SCOPE = decodeURIComponent(getQueryVariable("scope"));

    if (conf.GOOGLE_CLIENT_ID != null && conf.REDIRECT_URI != null && conf.SCOPE != null) {

        // Construct google URL and GO
        // COMMENT it 
        var oauthUrl = "https://accounts.google.com/o/oauth2/auth?response_type=code&access_type=offline&prompt=consent%20select_account&client_id=" + encodeURIComponent(conf.GOOGLE_CLIENT_ID)
         + "&redirect_uri=" + encodeURIComponent(conf.REDIRECT_URI)
          + "&scope=" + encodeURIComponent(conf.SCOPE);


        // This is needed only for `pebble emu-app-config` command
        var returnTo = getQueryVariable('return_to');
        if (returnTo != null) {
            oauthUrl += "&state=" + encodeURIComponent(returnTo);
        }
        
        loga("Google URL=" + oauthUrl);
        // setlink("GoTo Google URL", oauthUrl);

        return oauthUrl;

    }   

    return null;
}


function goToGoogleLogin() {

    var oauthUrl = getGoogleLoginUrl();

    if (oauthUrl) {
        setTimeout(function() {
            window.location.href = oauthUrl;
        }, 1000);
    }

}


function log(str) {
    if (DEBUG) {
        $("#logs").css("display", "block");
        $("#logs").text(str);
    }
}


function loga(str) {
    log($("#logs").text() + "\n\n" + str);
}



function getQueryVariable(variable, defaultValue) {
    var defaultValue = defaultValue || null;
    var query = window.location.search.substring(1);
    var vars = query.split("&");
    for (var i=0; i < vars.length; i++) {
      var pair = vars[i].split("=");
      if (pair[0] == variable) {
        return decodeURIComponent(pair[1]);
      }
    }
    return defaultValue;
}

function composeReturnUrl(obj) {
    //Determine closeUrl - needed to get return_to link in case of using `pebble emu-app-config`
    var return_to = getQueryVariable('state');
    //alert('return_to=' + return_to + JSON.stringify(obj));
    if (!return_to) {
        return_to = getQueryVariable('return_to', 'pebblejs://close#')
    }

    //var closeURL = "pebblejs://close#" + JSON.stringify(obj);
    var closeURL = return_to + JSON.stringify(obj);
    //alert('return_to=' + closeURL);    

    return closeURL;
}


function endConfigurationWithParam(name, value) {
    var obj = {};
    obj[name] = value;
    obj["fetchAllSelectedCalendars"] = true;

/*
    //Determine closeUrl - needed to get return_to link in case of using `pebble emu-app-config`
    var return_to = getQueryVariable('state', 'pebblejs://close#');
    //alert('return_to=' + return_to + JSON.stringify(obj));

    //var closeURL = "pebblejs://close#" + JSON.stringify(obj);
    var closeURL = return_to + JSON.stringify(obj);
    //alert('return_to=' + closeURL);
*/

    var closeURL = composeReturnUrl(obj);

    loga("closeURL = " + closeURL);
    window.location.href = closeURL;
}


function showLoginToGoogleView() {

    ga('send', 'pageview', '/view-google-login');

    var source = $("#google-login-view").html();
    var template = Handlebars.compile(source);
    var html = template();

    $("#content").html(html);

    var googleConnectButton = $("#google-connect-button");
    googleConnectButton.on('click', function() {
        console.log("Google Authorize Button Click");
        console.log("location = ", getGoogleLoginUrl());

        showLoadingView("Redirecting to Google ...");

        ga('send', 'pageview', '/button-google-login', {
            hitCallback: createFunctionWithTimeout(function() {
                goToGoogleLogin();
            })
        });

    });
}

function showSettingsView(userInfo) {

    ga('send', 'pageview', '/view-settings');

    console.log('userInfo =', userInfo);

    var source = $("#settings-view").html();
    var template = Handlebars.compile(source);
    var html = template();

    $("#content").html(html);


    var unlinkGoogleButton = $("#unlink-google-button");
    unlinkGoogleButton.on('click', function() {
        console.log("Google Unlink Button Click");

        // ga('send', 'pageview', '/button-google-unlink');

        showLoadingView("Unlinking Google account ...");

        ga('send', 'pageview', '/button-google-unlink', {
            hitCallback: createFunctionWithTimeout(function() {
                endConfigurationWithParam("unlink", true);
            })
        });
        

        
        //console.log("location = ", getGoogleLoginUrl());
        //goToGoogleLogin();
    });

}

function showLoadingView(message) {
    var source = $("#loading-view").html();
    var template = Handlebars.compile(source);
    var html = template({message: message});

    $("#content").html(html);

}



// MAIN function
(function() {

    var loc = window.location;
    loga("location = " + loc);

    // Check for `error` from Google
    // if error - finish configuration and return to pebble
    var error = getQueryVariable("error");
    if (error != null) {
        loga("error = " + error);
        // ga('send', 'pageview', '/result-google-error');
        ga('send', 'pageview', '/result-google-error', {
            hitCallback: createFunctionWithTimeout(function() {
                endConfigurationWithParam("error", error);
            })
        });
        return;
    }

    // If `code` then it is callback from Google
    // Finish config and go directly to the pebble to store it
    var code = getQueryVariable("code");
    if (code != null) {
        // ga('send', 'pageview', '/result-google-code');
        ga('send', 'pageview', '/result-google-code', {
            hitCallback: createFunctionWithTimeout(function() {
                endConfigurationWithParam("code", code);
            })
        });
        return;
    }


    // If !error and !code this is open Settings page
    // check for codePresent first
    var codePresent = getQueryVariable("cp");
    if (!codePresent) {
        // There is no codePresent then go to Google!
        loga("No Code - need to GoogleLogin");
        showLoginToGoogleView();
        return;
    }

    // We have Google authorize (codePresent)
    var storedUserInfo = getQueryVariable("sui");
    if (storedUserInfo) {

        // HACK: For some reason we need to double decode...
        // can't find the reason
        storedUserInfo = decodeURIComponent(storedUserInfo);
        
        loga("Show Config with userInfo settings");
        loga("storedUserInfo = " + storedUserInfo);
        storedUserInfo = JSON.parse(storedUserInfo);
        loga("storedUserInfo = " + storedUserInfo);
        console.log(storedUserInfo);
        //return;
    }

    showSettingsView(storedUserInfo);


})();



            

/*
            var DEBUG = 1;

            var loc = window.location;

            var primaryCalendar = getQueryVariable("primaryCalendar");
            if (primaryCalendar != null) {
                loga("primaryCalendar = " + primaryCalendar);
            }

            loga("location = " + loc);

            var code = getQueryVariable("code");
            if (code != null) {
                endConfigurationWithParam("code", code);
            } else {
                // No "code" is present.

                
                // Check for error first
                var error = getQueryVariable("error");
                if (error != null) {
                    loga("error = " + error);
                    endConfigurationWithParam("error", error);
                } else {
                    // Go to Google if no error

                    var conf = {}; //JSON.parse(json);

                    //alert('return_to=' + getQueryVariable('return_to'))

                    conf.GOOGLE_CLIENT_ID = decodeURIComponent(getQueryVariable("client_id"));
                    conf.REDIRECT_URI = decodeURIComponent(getQueryVariable("redirect_uri"));
                    conf.SCOPE = decodeURIComponent(getQueryVariable("scope"));

                    if (conf.GOOGLE_CLIENT_ID != null && conf.REDIRECT_URI != null && conf.SCOPE != null) {

                        // Construct google URL and GO
                        // COMMENT it 
                        var oauthUrl = "https://accounts.google.com/o/oauth2/auth?response_type=code&access_type=offline&prompt=consent%20select_account&client_id=" + encodeURIComponent(conf.GOOGLE_CLIENT_ID)
                         + "&redirect_uri=" + encodeURIComponent(conf.REDIRECT_URI)
                          + "&scope=" + encodeURIComponent(conf.SCOPE);


                        // This is needed only for `pebble emu-app-config` command
                        var returnTo = getQueryVariable('return_to');
                        if (returnTo != null) {
                            oauthUrl += "&state=" + encodeURIComponent(returnTo);
                        }

                          
                        
                        loga("GoTo Google URL=" + oauthUrl);
                        // setlink("GoTo Google URL", oauthUrl);
                        setTimeout(function() {
                            window.location.href = oauthUrl;
                        }, 1000);
                        

                    }

                }

            }

*/

