# My Calendar - Pebble watchface/watchapp

Connects to Google Calendar and shows the last three events from default Google calendar on your Pebble Smartwatch.

App URL: https://apps.getpebble.com/applications/5425862155f4a0a817000101

# Contributions

This code is for reference only and for enthusiasts who are still tinkering their Pebbles :) Enjoy!

# Project dependencies
* UglifyJS - available to install via NPM ("npm install uglify-js@1 -g"). Check online docs for the most recent version
* jshint

# Build, Deploy, Install

To build pebble app and copy configuration.html files just Run from root:
```
./do-all
```

If needed set up env variable PEBBLE_PHONE with your iPhone IP
```
export PEBBLE_PHONE=<your phone's IP>
```

If encounter error on sending files through rsync and error smth with broad permissions just fix it with
```
chmod 600 pebble_key
```

# Submit to AppStore

GifMaker
http://gifmaker.me/

# Pebble Doc

Pebble Developer Guide [Doc]
https://developer.getpebble.com/2/guides/

Russian Firmaware [Tool]
http://pebblebits.com/firmware/

# Google Doc

Using OAuth 2.0 to Access Google APIs [Doc]
https://developers.google.com/accounts/docs/OAuth2

Google Accounts Authentication and Authorization
https://developers.google.com/accounts/docs/OAuth2Login


Using OAuth 2.0 for Web Server Applications [Doc]
https://developers.google.com/accounts/docs/OAuth2WebServer

Google Calendar API [Doc]
https://developers.google.com/google-apps/calendar/

OAuth 2.0 Playground [Tool]
https://developers.google.com/oauthplayground/

Google Authorize Access Page [Tool - revoke tokens here]
https://accounts.google.com/IssuedAuthSubTokens
or
https://security.google.com/settings/security/permissions?pli=1

Google Developer Console [Console]
https://console.developers.google.com
