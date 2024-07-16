#!/usr/bin/env python3

# based on https://gist.github.com/mdonkers/63e115cc0c79b4f6b8b3a6b797e485c7

from http.server import BaseHTTPRequestHandler, HTTPServer
import logging
import json
import threading

from dotenv import load_dotenv
import os

from arduino_iot_cloud import ArduinoCloudClient

if __name__ == '__main__':
    load_dotenv()
    logging.basicConfig(level=logging.INFO)

    logging.info('connecting to Arduino IoT Cloud')
    client = ArduinoCloudClient(
        device_id=os.getenv('DEVICE_ID'),
        username=os.getenv('DEVICE_ID'),
        password=os.getenv('DEVICE_KEY')
    )
    client.register('temperature', value=None)
    client.register('humidity', value=None)
    def start_arduino_cloud():
        client.start()
    threading.Thread(target=start_arduino_cloud).start()
    
    class RequestHandler(BaseHTTPRequestHandler):
        def _set_response(self, code=200, message='Success'):
            self.send_response(code, f'{{"status":"{code}","message":"{message}"}}') # will probably be disregarded anyway
            self.send_header('Content-Type', 'application/json')
            self.end_headers()

        def do_POST(self):
            logging.info(f'POST {self.path}')

            if self.path == '/sensor':
                data = json.loads(self.rfile.read(int(self.headers['Content-Length'])).decode('utf-8'))
                temp = data['temperature']; hum = data['humidity']
                logging.info(f'temperature: {temp} C, humidity: {hum} %')
                client['temperature'] = temp; client['humidity'] = hum # update to Arduino IoT Cloud
            else: self._set_response(404, 'Not Found')

    httpd = HTTPServer(('', int(os.getenv('PORT'))), RequestHandler)
    logging.info('HTTP server started')

    try:
        httpd.serve_forever()
    finally:
        httpd.server_close()
        logging.info('HTTP server stopped')