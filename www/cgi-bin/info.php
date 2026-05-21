<?php
// PHP-CGI automatically outputs the 'Content-Type' header.
// We just need to output the body.

$method = $_SERVER['REQUEST_METHOD'];
$query = isset($_SERVER['QUERY_STRING']) ? $_SERVER['QUERY_STRING'] : '';

echo "<!DOCTYPE html>\n<html>\n<head><title>PHP CGI Response</title></head>\n<body>\n";
echo "<h2>PHP CGI is Working Successfully!</h2>\n";
echo "<ul>\n";
echo "<li><strong>Method:</strong> " . htmlspecialchars($method) . "</li>\n";
echo "<li><strong>Query String:</strong> " . htmlspecialchars($query) . "</li>\n";
echo "<li><strong>Server Software:</strong> Webserv/1.0</li>\n";
echo "</ul>\n";

if ($method === 'POST') {
    // Read raw body for POST requests
    $post_data = file_get_contents("php://input");
    echo "<h3>Received POST Data:</h3>\n";
    echo "<pre>" . htmlspecialchars($post_data) . "</pre>\n";
}

echo "</body>\n</html>\n";
?>
