/**
 * Copyright 2019 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

'use strict';

const functions = require('firebase-functions');
const {smarthome} = require('actions-on-google');
const {google} = require('googleapis');
const util = require('util');
// Initialize Homegraph
const auth = new google.auth.GoogleAuth({
  keyFile: 'smart-home-key.json',
  scopes: ['https://www.googleapis.com/auth/homegraph']
});

const homegraph = google.homegraph({
  version: 'v1',
  auth: auth
});

const app = smarthome({
  debug: false,
});

// should be in database
let global_state = false
const client_id = 'sgvsvsv';
const client_secret = 'abc';
const auth_code = 'xxxxxx';
const atoken = '123access';
const rtoken = '123refresh';
const UserId = '123';

// checks header for valid token needs to be done by user, but its only me
const check_headers = (headers) => {
  return headers.authorization.split(" ").pop() == atoken
}
exports.fakeauth = functions.https.onRequest((request, response) => {
  const responseurl = util.format('%s?code=%s&state=%s',
    decodeURIComponent(request.query.redirect_uri), auth_code,
    request.query.state);
    const google_host = (new URL(decodeURIComponent(request.query.redirect_uri))).hostname
    if(google_host == "oauth-redirect.googleusercontent.com" || google_host == "oauth-redirect-sandbox.googleusercontent.com")
      return response.redirect(responseurl);
    return response.status(HTTP_STATUS_UNAUTHORIZED);
});

exports.faketoken = functions.https.onRequest((request, response) => {
  const grantType = request.query.grant_type
    ? request.query.grant_type : request.body.grant_type;
  const secondsInDay = 86400; // 60 * 60 * 24
  const HTTP_STATUS_OK = 200;
  const HTTP_STATUS_UNAUTHORIZED = 401;

  if(request.body.client_id !== client_id || request.body.client_secret !== client_secret)
    response.status(HTTP_STATUS_UNAUTHORIZED);

  if(request.body.authorization_code !== auth_code)
    response.status(HTTP_STATUS_UNAUTHORIZED);

  let obj;
  if (grantType === 'authorization_code') {
    obj = {
      token_type: 'bearer',
      access_token: atoken,
      refresh_token: rtoken,
      expires_in: secondsInDay,
    };
  } else if (grantType === 'refresh_token') {
    if(request.body.authorization_code != rtoken)
      response.status(HTTP_STATUS_UNAUTHORIZED);
    obj = {
      token_type: 'bearer',
      access_token: atoken,
      expires_in: secondsInDay,
    };
  }
  response.status(HTTP_STATUS_OK)
    .json(obj);
});

app.onDisconnect(() => {
  return {}
});

app.onSync((body, headers) => {
  if (!check_headers(headers))
    return {};
  return {
    requestId: body.requestId,
    payload: {
      agentUserId: UserId,
      devices: [{
        id: 'light',
        type: 'action.devices.types.LIGHT',
        traits: [
          'action.devices.traits.OnOff',
        ],
        name: {
          defaultNames: ['Kitchen light'],
          name: 'kitchen light',
          nicknames: ['light'],
        },
        willReportState: true,
      }],
    },
  };
});

app.onQuery(async (body, headers) => {
  if (!check_headers(headers))
    return {};
  const {requestId} = body;
  const payload = {
    devices: {},
  };
  const queryPromises = [];
  const intent = body.inputs[0];
  for (const device of intent.payload.devices) {
    const deviceId = device.id;
    queryPromises.push(() => {
        payload.devices[deviceId] = global_state;
      }
    );
  }
  await Promise.all(queryPromises)
  return {
    requestId: requestId,
    payload: payload,
  };
});

const updateDevice = async (execution,deviceId) => {
  const {params,command} = execution;
  switch (command) {
    case 'action.devices.commands.OnOff':
      global_state = {on: params.on};
      break;
  }
  console.log(global_state.on)

  return global_state;
};

app.onExecute(async (body, headers) => {
  if (!check_headers(headers))
    return {};
  const {requestId} = body;
  // Execution results are grouped by status
  const result = {
    ids: [],
    status: 'SUCCESS',
    states: {
      online: true,
    },
  };

  const executePromises = [];
  const intent = body.inputs[0];
  for (const command of intent.payload.commands) {
    for (const device of command.devices) {
      for (const execution of command.execution) {
        executePromises.push(
          updateDevice(execution,device.id)
            .then((data) => {
              result.ids.push(device.id);
              Object.assign(result.states, data);
            })
            .catch(() => console.error(`Unable to update ${device.id}`))
        );
      }
    }
  }

  await Promise.all(executePromises)
  return {
    requestId: requestId,
    payload: {
      commands: [result],
    },
  };
});


exports.smarthome = functions.https.onRequest(app);

exports.requestsync = functions.https.onRequest(async (request, response) => {
  response.set('Access-Control-Allow-Origin', '*');
  //console.info('Request SYNC for user 123');
  try {
    const res = await homegraph.devices.requestSync({
      requestBody: {
        agentUserId: UserId
      }
    });
    //console.info('Request sync response:', res.status, res.data);
    response.json(res.data);
  } catch (err) {
    console.error(err);
    response.status(500).send(`Error requesting sync: ${err}`)
  }
});

const update_state = async (onlines, val, deviceId) => {
  const requestBody = {
    requestId: 'ff36a3cc', /* Any unique ID */
    agentUserId: UserId, /* Hardcoded user ID */
    payload: {
      devices: {
        states: {
          /* Report the current state of our light */
          [deviceId]: {
            on: val,
            online: onlines,
          },
        },
      },
    },
  };

  const res = await homegraph.devices.reportStateAndNotification({
    requestBody
  });
};
