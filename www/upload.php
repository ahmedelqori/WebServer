<?php
if ($_SERVER['REQUEST_METHOD'] == 'POST') {
    // Check if a file has been uploaded
    if (isset($_FILES['file']) && $_FILES['file']['error'] == UPLOAD_ERR_OK) {
        // Get file details
        $file_tmp = $_FILES['file']['tmp_name'];
        $file_name = $_FILES['file']['name'];
        $upload_dir = '/path/to/upload/directory/';
        $file_path = $upload_dir . $file_name;
        
        // Move the uploaded file to the designated directory
        if (move_uploaded_file($file_tmp, $file_path)) {
            echo "<h1>File uploaded successfully!</h1>";
            echo "<p>File saved as: " . htmlspecialchars($file_name) . "</p>";
        } else {
            echo "<h1>File upload failed.</h1>";
        }
    } else {
        echo "<h1>No file uploaded or there was an upload error.</h1>";
    }
} else {
    // Display the upload form
    echo '<h1>Upload a file</h1>';
    echo '<form action="" method="POST" enctype="multipart/form-data">';
    echo '<input type="file" name="file" />';
    echo '<input type="submit" value="Upload" />';
    echo '</form>';
}
?>
