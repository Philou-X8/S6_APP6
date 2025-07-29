import threading
import asyncio
from http.server import HTTPServer, BaseHTTPRequestHandler
from aiocoap import *

DB_FILE = 'DB.txt'

async def coap_observe():
    protocol = await Context.create_client_context()
    request = Message(code=GET, uri='coap://192.168.85.122:5683/user_event', observe=0)
    pr = protocol.request(request)
    async for response in pr.observation:
        data = response.payload.decode()
        print("Observed update:", data)
        with open(DB_FILE, 'a', encoding='utf-8') as f:
            f.write(data + '\n')

def start_coap_observer_loop():
    asyncio.run(coap_observe())

class FileHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/':
            try:
                with open(DB_FILE, 'r', encoding='utf-8') as f:
                    content = f.read()
            except FileNotFoundError:
                content = "DB.txt not found."
            content_bytes = content.encode('utf-8')
            self.send_response(200)
            self.send_header('Content-Type', 'text/plain; charset=utf-8')
            self.send_header('Content-Length', str(len(content_bytes)))
            self.end_headers()
            self.wfile.write(content_bytes)
        else:
            self.send_error(404)

def run_http_server():
    server = HTTPServer(('0.0.0.0', 8080), FileHandler)
    print("HTTP server running at http://0.0.0.0:8080")
    server.serve_forever()

if __name__ == '__main__':
    with open('DB.txt','w') :
        pass
    # Start CoAP observer in a thread
    threading.Thread(target=start_coap_observer_loop, daemon=True).start()
    # Start HTTP server in main thread
    run_http_server()
