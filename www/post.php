<?php
// Set headers first
header('Content-Type: text/plain');
header('X-Powered-By: PHP/7.4.3');
header('Status: 200 OK');

// Debug logging
error_log("PHP Script Started");
error_log("REQUEST_METHOD: " . $_SERVER['REQUEST_METHOD']);

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    // Read raw POST data
    $raw_data = file_get_contents('php://input');
    error_log("Raw POST data: " . $raw_data);
    
    // Print headers and empty line to separate headers from body
    echo "Content-Type: text/plain\r\n";
    echo "Status: 200 OK\r\n";
    echo "\r\n"; // Empty line to separate headers from body
    
    // Send response body
    echo "Received POST data: " . $raw_data . "\n";
    
    if (!empty($_POST)) {
        echo "Form data received:\n";
        foreach ($_POST as $key => $value) {
            echo "$key: $value\n";
        }
    }
} else {
    // For non-POST requests
    echo "Content-Type: text/plain\r\n";
    echo "Status: 405 Method Not Allowed\r\n";
    echo "\r\n";
    echo "Only POST requests are accepted";
}
?>