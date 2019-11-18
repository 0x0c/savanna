import http.server
import socketserver
from urllib.parse import urlparse

LISTEN_PORT = 8080


class ServerHandler(http.server.SimpleHTTPRequestHandler):

    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        self.wfile.write(b"<h1>It works!</h1>")
        query = urlparse(self.path).query
        print(self.headers)
        print("---Query---")
        print(query)

    def do_POST(self):
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
        content_len = int(self.headers.get("content-length"))
        req_body = self.rfile.read(content_len).decode("utf-8")
        print(self.headers)
        print("---Body---")
        print(req_body)


if __name__ == "__main__":
    HOST, PORT = '', LISTEN_PORT

    with socketserver.TCPServer((HOST, PORT), ServerHandler) as server:
        server.serve_forever()
