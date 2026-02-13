GET http://localhost:8080/
HTTP 200
[Asserts]
header "Content-Type" == "text/html"
body contains "Hello"
