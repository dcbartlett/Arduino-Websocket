/*
    Arduino-Websocket, a websocket implementation for Arduino
    Copyright 2012, 2013 Ahti Legonkov, Raido Lepp

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "WebSocketServer.h"

#include "sha1.h"
#include "Base64.h"
#include "ws_debug.h"
#include "ws_string.h"

using namespace websocket;
typedef websocket::String wsString;

ServerHandshake::ServerHandshake(Socket& socket, String const& acceptOrigin)
    : socket_(socket)
    , acceptOrigin_(acceptOrigin)
{ }

bool ServerHandshake::run()
{
    // If there is a connected client->
    if (socket_.connected()) {
        // Check request and look for websocket handshake
        wsDebug() << F("Client connected\n");
        if (analyzeRequest()) {
                wsDebug() << F("Websocket established\n");
                return true;

        } else {
            // Might just need to break until out of socket_ loop.
            wsDebug() << F("Disconnecting client\n");
            disconnect();

            return false;
        }
    } else {
        return false;
    }
}

bool ServerHandshake::analyzeRequest()
{
    // Use String library to do some sort of read() magic here.
    String temp;

    int bite;
    bool foundupgrade = false;
    String oldkey[2];
    wsString newkey;

    wsDebug() << F("Analyzing request headers\n");

    // TODO: More robust string extraction
    while ((bite = socket_.read()) != -1) {

        temp += (char)bite;

        if ((char)bite == '\n') {
            wsDebug() << "Got Line: " << temp << "\n";

            // TODO: Should ignore case when comparing and allow 0-n whitespace after ':'. See the spec:
            // http://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html
            if (!foundupgrade && temp.startsWith("Upgrade: WebSocket")) {
                // OK, it's a websockets handshake for sure
                foundupgrade = true;
            } else if (!foundupgrade && temp.startsWith("Upgrade: websocket")) {
                foundupgrade = true;
            } else if (temp.startsWith("Origin: ")) {
                origin_ = temp.substring(8,temp.length() - 2); // Don't save last CR+LF
            } else if (temp.startsWith("Host: ")) {
                host_ = temp.substring(6,temp.length() - 2); // Don't save last CR+LF
            } else if (temp.startsWith("Sec-WebSocket-Key1: ")) {
                oldkey[0]=temp.substring(20,temp.length() - 2); // Don't save last CR+LF
            } else if (temp.startsWith("Sec-WebSocket-Key2: ")) {
                oldkey[1]=temp.substring(20,temp.length() - 2); // Don't save last CR+LF
            } else if (temp.startsWith("Sec-WebSocket-Key: ")) {
                newkey=temp.substring(19,temp.length() - 2); // Don't save last CR+LF
            }
            temp = "";
        }

        if (!socket_.available()) {
          delay(20);
        }
    }

    if (!socket_.connected()) {
        return false;
    }

    temp += 0; // Terminate string

    // Assert that we have all headers that are needed. If so, go ahead and
    // send response headers.
    if (foundupgrade) {

        if (newkey.length() > 0) {

            Sha1 sha;
            sha.update(reinterpret_cast<uint8_t const*>(newkey.c_str()), newkey.length());
            // add the magic string
            uint8_t const magic[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
            sha.update(magic, sizeof(magic)-1);

            uint8_t result[21];
            sha.finish(result);

            uint8_t b64Result[30];
            uint16_t const len = base64_encode(b64Result, result, 20);

            socket_.print(F("HTTP/1.1 101 Web Socket Protocol Handshake" CRLF
                            "Upgrade: websocket" CRLF
                            "Connection: Upgrade" CRLF
                            "Sec-WebSocket-Accept: "));
            socket_.write(b64Result, len);
            socket_.print(CRLF CRLF);

            return true;
        } else {
            // something went horribly wrong
            return false;
        }
    } else {
        // Nope, failed handshake. Disconnect
        wsDebug() << F("Header mismatch\n");
        return false;
    }
}

void ServerHandshake::disconnect()
{
    wsDebug() << F("Terminating socket\n");

    // Should send 0x8700 to server to tell it I'm quitting here.
    socket_.write((uint8_t) 0x87);
    socket_.write((uint8_t) 0x00);
    
    socket_.flush();
    delay(10);
    socket_.stop();
}
