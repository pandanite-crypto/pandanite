from http import server 

class CrossOriginHandler(server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_cross_origin_headers()

        server.SimpleHTTPRequestHandler.end_headers(self)

    def send_cross_origin_headers(self):
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")


if __name__ == '__main__':
    server.test(HandlerClass=CrossOriginHandler)