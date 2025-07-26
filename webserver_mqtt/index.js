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

// Express dependency for web UI
const express = require('express')
const exp_app = express()
const exp_port = 3000

function ResetDB(rst=true){
    if (!rst) return;

    fs.writeFile(file_path, "", (err) => {
        if (err) {
            console.error('Could not reset the DB file:', err);
        }
    });
}

function AddToDB(str){
    fs.appendFile(file_path, str.toString() + "\n", (err) => {
        if (err) {
            console.error('Error writing to file:', err);
        }
    });
}

mqtt_client.on("connect", () => {

    ResetDB();

    mqtt_client.subscribe(message_path, (err) => {

        if (!err) {
            mqtt_client.publish(message_path, "Webserver_MQTT started");
        }
        else{
            console.log("failed to subscribe to: " + message_path);
        }

    });
});

mqtt_client.on("message", (topic, message) => {
    // message is Buffer
    console.log(message_path + ": " + message.toString());
    if(message.toString().startsWith("USER_EVENT:")){

        AddToDB(message.toString().substring(11));
    }
    
    //client.end(); // enabling this shut down the server after a message
});

exp_app.get('/', (req, res) => {
    fs.readFile(file_path, 'utf8', (err, data) => {
        if (!err) {
            // send DB content to web page

            var formated = "<p>" + data.replaceAll("\n", "<br>") + "</p>";

            //console.log(formated);
            res.send(formated);
        }
        else{
            console.log('Cannot read file to forward to web page:', err);
        }

    });
})

exp_app.listen(exp_port, () => {
    console.log(`Example app listening on port ${exp_port}`)
})
