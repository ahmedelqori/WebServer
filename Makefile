# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: ael-qori <ael-qori@student.1337.ma>        +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/12/19 15:51:13 by ael-qori          #+#    #+#              #
<<<<<<< HEAD
#    Updated: 2025/01/23 22:33:43 by ael-qori         ###   ########.fr        #
=======
#    Updated: 2025/01/22 15:39:33 by mbentahi         ###   ########.fr        #
>>>>>>> dff41d90fd5470bef0ae97c2f7401bfecba8c230
#                                                                              #
# **************************************************************************** #

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
