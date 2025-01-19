<?php
// This is a basic PHP script that mimics CGI GET behavior
header("Content-Type: text/plain"); // Set content type for plain text output

// Get query parameters
if ($_SERVER['REQUEST_METHOD'] == 'GET') {
    // Retrieve query parameters via $_GET array
    if (count($_GET) > 0) {
        // Iterate through each parameter and output its value
        foreach ($_GET as $key => $value) {
            echo "Received parameter: $key = $value\n";
        }
    } else {
        echo "No GET parameters received.\n";
    }
} else {
    echo "This script only handles GET requests.\n";
}
?>
