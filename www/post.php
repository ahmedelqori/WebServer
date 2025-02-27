<?php
// URL of the CGI script
$cgiUrl = "http://127.0.0.5:8888/post.php";

// Data to send via POST
$postData = array(
    'param1' => 'value1',
    'param2' => 'value2',
);

// Create a POST string from the data
$postDataString = http_build_query($postData);

// Set the necessary HTTP headers
$options = array(
    'http' => array(
        'method'  => 'POST',
        'header'  => "Content-type: application/x-www-form-urlencoded\r\n",
        'content' => $postDataString,
    ),
);

// Create a context with the HTTP options
$context = stream_context_create($options);

// Send the POST request and get the response
$response = file_get_contents($cgiUrl, false, $context);

// Check if the response was successfully received
if ($response === FALSE) {
    die('Error occurred while making the POST request');
}

// Output the response from the CGI script
echo "Response from CGI script: " . $response;
?>
