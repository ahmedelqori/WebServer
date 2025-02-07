<?php
// Set a new cookie if one doesn't exist
if (!isset($_COOKIE['user'])) {
    setcookie('user', 'firstVisit', time() + 3600); // Expires in 1 hour
    echo "Cookie set for first time visitor\n";
} else {
    echo "Welcome back! Your cookie value is: " . $_COOKIE['user'] . "\n";
    // Update the cookie
    setcookie('user', 'returnVisitor', time() + 3600);
}

// Display all cookies
echo "\nAll cookies:\n";
foreach ($_COOKIE as $key => $value) {
    echo "$key: $value\n";
}
?>