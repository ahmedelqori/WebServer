<?php
// Enable error reporting for debugging
error_reporting(E_ALL);
ini_set('display_errors', 1);

// Function to handle different CGI actions
function handleCGIRequest() {
    // Check if there's a specific action requested
    $action = isset($_GET['action']) ? $_GET['action'] : '';
    
    switch($action) {
        case 'execute':
            // Simulate an execution action
            $command = isset($_GET['command']) ? $_GET['command'] : '';
            if (!empty($command)) {
                // WARNING: Be extremely cautious with exec() in production
                // This is for demonstration purposes only
                $output = [];
                $returnVar = 0;
                exec($command, $output, $returnVar);
                return [
                    'status' => 'success',
                    'output' => $output,
                    'return_code' => $returnVar
                ];
            }
            return ['status' => 'error', 'message' => 'No command provided'];
        
        case 'info':
            // Provide server and CGI information
            return [
                'status' => 'success',
                'server_software' => $_SERVER['SERVER_SOFTWARE'],
                'server_name' => $_SERVER['SERVER_NAME'],
                'gateway_interface' => $_SERVER['GATEWAY_INTERFACE'] ?? 'Not specified',
                'request_method' => $_SERVER['REQUEST_METHOD'],
                'query_string' => $_SERVER['QUERY_STRING'] ?? 'No query string'
            ];
        
        default:
            return ['status' => 'error', 'message' => 'Invalid action'];
    }
}

// Process the request
$response = handleCGIRequest();

// Determine output type
$format = isset($_GET['format']) ? $_GET['format'] : 'html';

if ($format === 'json') {
    // JSON output for API-like usage
    header('Content-Type: application/json');
    echo json_encode($response);
    exit;
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>CGI Test Implementation</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 800px; margin: 0 auto; padding: 20px; }
        pre { background-color: #f4f4f4; padding: 10px; border-radius: 5px; }
        .result { margin-top: 20px; }
    </style>
</head>
<body>
    <h1>CGI Test Script</h1>
    
    <div class="test-form">
        <h2>Execute Command</h2>
        <form method="GET">
            <input type="hidden" name="action" value="execute">
            <label for="command">Enter Command:</label>
            <input type="text" name="command" id="command" placeholder="e.g., ls -l or dir">
            <button type="submit">Execute</button>
        </form>
    </div>

    <?php if ($format !== 'json'): ?>
    <div class="result">
        <h2>Result</h2>
        <?php if ($response['status'] === 'success'): ?>
            <?php if (isset($response['output'])): ?>
                <pre><?php print_r($response['output']); ?></pre>
            <?php else: ?>
                <pre><?php print_r($response); ?></pre>
            <?php endif; ?>
        <?php else: ?>
            <p>Error: <?php echo $response['message']; ?></p>
        <?php endif; ?>
    </div>
    <?php endif; ?>

    <div class="links">
        <h2>Quick Links</h2>
        <ul>
            <li><a href="?action=info">Get Server Info</a></li>
            <li><a href="?action=info&format=json">Get Server Info (JSON)</a></li>
        </ul>
    </div>
</body>
</html>