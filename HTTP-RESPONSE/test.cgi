#!/usr/bin/php

<?php

# Set content type header
header('Content-Type: text/plain');

# // Check if request method is POST
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    # // Handle form data
    if (isset($_POST['data'])) {
        echo "Received data: " . $_POST['data'] . "\n";
    } else {
        echo "No data received in POST request\n";
    }
    
    # // Print environment variables for debugging
    echo "\nEnvironment variables:\n";
    foreach ($_SERVER as $key => $value) {
        echo "$key: $value\n";
    }
}
?>