//-------------------------------
// S6 - APP6
// BREL0901
// DURP2003
//-------------------------------

// File system (fake DB)
const fs = require("fs");
const readline = require("readline");
const file_path = "DB.txt";

// MQTT dependency
const mqtt = require("mqtt");
const mqtt_client = mqtt.connect("mqtt://localhost");
const message_path = "user_event";

//coap
const { CoapClient } = require("node-coap-client");

// Express dependency for web UI
const express = require('express')
const exp_app = express()
const exp_port = 3001

function ResetDB(rst = true) {
    if (!rst) return;

    fs.writeFile(file_path, "", (err) => {
        if (err) {
            console.error('Could not reset the DB file:', err);
        }
    });
}

function AddToDB(str) {
    fs.appendFile(file_path, str.toString() + "\n", (err) => {
        if (err) {
            console.error('Error writing to file:', err);
        }
    });
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------ MQTT
//------------------------------------------------------------------------------

mqtt_client.on("connect", () => {

    ResetDB();
    mqtt_client.subscribe(message_path, (err) => {

        if (!err) {
            mqtt_client.publish(message_path, "Webserver_Control started");
        }
        else {
            console.log("failed to subscribe to: " + message_path);
        }

    });
});

mqtt_client.on("message", (topic, message) => {
    // message is Buffer
    console.log(message_path + ": " + message.toString());
    //AddToDB(message);

    //client.end(); // enabling this shut down the server after a message
});


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------ websocket
//------------------------------------------------------------------------------

// Creates a new WebSocket connection to the specified URL.
let socket = new WebSocket('ws://PLACEHOLDER:80/ws'); // replace PLACEHOLDER with ESP32 IP adress <-----------
// Executes when the connection is successfully established.
socket.addEventListener('open', event => {
    console.log('WebSocket connection established!');
    // Sends a message to the WebSocket server.
    socket.send('Hello Server!');
});
// Listen for messages and executes when a message is received from the server.
socket.addEventListener('message', event => {
    console.log('Webserver say: ', event.data);

    // received event from websocket
    if (event.data.toString().startsWith("USER EVENT:")) {
        let db_entry = Date.now().toString(); // get current timestamp
        db_entry = db_entry + "," + event.data.toString().substring(11); // add timestamp to user event entry

        AddToDB(db_entry); // add to DB

        mqtt_client.publish(message_path, "USER_EVENT:" + db_entry); // sent to mqtt broker

        // Construct payload
        const payload = Buffer.from("USER_EVENT:"+db_entry);

        // Send PUT request
        CoapClient.request(
            "coap://PLACEHOLDER:PORT/user_event",
            "put",
            payload,
            {
                confirmable: false,
                keepAlive: false,
                reuseSocket: true
            }
        )
            .then(response => {
                console.log("Response code:", response.code.toString());
                console.log("Payload:", response.payload.toString());
            }).catch(err => {
                console.error("Failed:", err);
            });
    }
});
// Executes when the connection is closed, providing the close code and reason.
socket.addEventListener('close', event => {
    console.log('WebSocket connection closed:', event.code, event.reason);
});
// Executes if an error occurs during the WebSocket communication.
socket.addEventListener('error', error => {
    console.error('WebSocket error:', error);
});


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------ web page
//------------------------------------------------------------------------------

exp_app.get('/', (req, res) => {
    fs.readFile(file_path, 'utf8', (err, data) => {
        if (!err) {
            // send DB content to web page

            var formated = "<p>" + data.replaceAll("\n", "<br>") + "</p>";

            //console.log(formated);
            res.send(formated);
        }
        else {
            console.log('Cannot read file to forward to web page:', err);
        }

    });
})

var led_state = 1;
exp_app.get('/led', (req, res) => {

    if (led_state) led_state = 0;
    else led_state = 1;
    //led_state = !led_state;
    console.log('loadded LED webpage, changing led to: ' + led_state.toString());
    res.send("changing led to: " + led_state.toString());

    socket.send("LED" + led_state.toString());
})

exp_app.listen(exp_port, () => {
    console.log(`Example app listening on port ${exp_port}`)
})
