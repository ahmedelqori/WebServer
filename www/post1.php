#!/usr/bin/php
<?php
// Set the appropriate Content-Type header
header('Content-Type: text/html');

// Force an error (e.g., divide by zero)
$number = 0;
$divide = 1 / $number;  // This will cause a division by zero error

echo "<h1>Success</h1>";
echo "<p>Result: $divide</p>";
?>