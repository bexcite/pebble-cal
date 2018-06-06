

var constants = (function($){

// Function to extend the object constants later with prod or prod configs
function extend (target, source) {
  target = target || {};
  for (var prop in source) {
    if (typeof source[prop] === 'object') {
      target[prop] = extend(target[prop], source[prop]);
    } else {
      target[prop] = source[prop];
    }
  }
  return target;
}

var configs = {};

// [DEVELOPMENT] My Calendar on pavel.bashmakov@gmail.com
configs.development = {
  CONFIG_GOOGLE_CLIENT_ID: "<GOOGLE_CLIENT_ID>",
  CONFIG_GOOGLE_CLIENT_SECRET: "<GOOGLE_CLIENT_SECRET>",
  CONFIG_GOOGLE_REDIRECT_URI: "<REDIRECT_URI>/configuration-dev.html",
  CONFIG_GOOGLE_SCOPE: "https://www.googleapis.com/auth/calendar.readonly",
  CONFIG_URL: "<CONFIG_URL>/configuration-dev.html"
};

// [PRODUCTION] My Calendar on pbashmakov@stanfy.com.ua
configs.production = {
  CONFIG_GOOGLE_CLIENT_ID: "<GOOGLE_CLIENT_ID_PROD>",
  CONFIG_GOOGLE_CLIENT_SECRET: "<GOOGLE_CLIENT_SECRET_PROD>",
  CONFIG_GOOGLE_REDIRECT_URI: "<REDIRECT_URI_PROD>/configuration.html",
  CONFIG_GOOGLE_SCOPE: "https://www.googleapis.com/auth/calendar.readonly",
  CONFIG_URL: "<CONFIG_URL_PROD>/configuration.html"
};


var constants = {};


constants.ENV = 'development';
// constants.ENV = 'production';

// Event struct meta data
constants.EVENT_STRUCT_START = 20;
constants.EVENT_STRUCT_SIZE = 4;

// Event types
constants.EVENT_TYPE_EMPTY = 0;
constants.EVENT_TYPE_DEFAULT = 1;
constants.EVENT_TYPE_ALL_DAY = 2;

//var SUMMARY_LENGTH = 40; // More than 40 chars could cause error in sending russian long events (WTF? - I don't know :)
constants.DAY_SPLITTER = false;

constants.RETRY_SEND_MAX_ATTEMPTS = 3;
constants.RETRY_SEND_TIMEOUT = 20000;


// More than 40 chars could cause error in sending russian long events (WTF? - I don't know :)
constants.SUMMARY_LENGTH = 40;

// If you need to test with real data on pebble emulator use `false` here and then `pebble emu-app-config` command to set up Settings page parameters
// Also is TEST_EVENTS = true and there is a valid Google Oauth code in posession of the app it will continue serve Google Calendar data.
constants.TEST_EVENTS = false;


/*
if (constants.ENV === 'development') {
  extend(constants, configs['development']);
} else if (constants.ENV === 'production') {
  extend(constants, configs['production']);
}
*/

// Select environment
extend(constants, configs[constants.ENV] || {});


return constants;


})();

//var SUMMARY_LENGTH = 40; // More than 40 chars could cause error in sending russian long events (WTF? - I don't know :)
