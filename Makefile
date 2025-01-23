
NAME = webserv
CXX = c++
CXXFLAGS = -g -fsanitize=address  -std=c++98 #-Wall -Wextra -Werrore -pedantic
RM = rm -rf

SRC = main.cpp ./ConfigParser/ConfigParser.cpp ./ConfigParser/UtilsParser.cpp ./Server/Server.cpp \
		Http/HttpParser.cpp Http/Request.cpp Http/Response.cpp Http/ServerUtils.cpp Http/RequestHandler.cpp \
		Cgi/Cgi.cpp ./Logger/Logger.cpp\


OBJ = $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean: 
	$(RM) $(OBJ)

fclean: clean
	$(RM) $(NAME)

re: fclean all
