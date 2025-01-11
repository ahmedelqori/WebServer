# Webserve

Webserve is a lightweight, high-performance HTTP server implemented in C++ that conforms to the HTTP/1.1 specification. It is designed to handle multiple concurrent requests efficiently, providing a solid foundation for building custom web server solutions.

---

# In Progress  
This project is actively under development. New features are being added, and optimizations are ongoing. Stay tuned for updates and improvements!  

---

## 🔥 Features (Planned and Implemented)

- **HTTP/1.1 Compliance**: Handles standard HTTP requests and responses.
- **Multi-Client Support**: Serves multiple users simultaneously.
- **Configuration Parsing**: Dynamic configuration via a custom config file.
- **Static File Hosting**: Serves static files like HTML, CSS, and JavaScript.
- **Error Handling**: Custom error pages for better user experience.
- **Request Routing**: Maps HTTP requests to specific handlers.
- **Logging**: Tracks server activity for debugging and monitoring.
- **CGI Support**: Execute server-side scripts.

---

## 💻 Technologies Used

- **C++**: Core language for the project.
- **POSIX Sockets**: For network communication.
- **Multi-threading**: To handle concurrent requests efficiently.

---

## 🚀 Getting Started  

### Prerequisites  
- C++ Compiler (e.g., g++, clang++)  
- GNU Make  

### Build Instructions  
1. Clone the repository:  
   ```bash
   git clone https://github.com/ahmedelqori/Webserve.git
   cd Webserve
