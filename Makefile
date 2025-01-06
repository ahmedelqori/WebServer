# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: ael-qori <ael-qori@student.1337.ma>        +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/12/19 15:51:13 by ael-qori          #+#    #+#              #
#    Updated: 2025/01/06 14:46:40 by ael-qori         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = webserv
CC = c++
CFLAGS = -std=c++98 #-fsanitize=address#-Wall -Wextra -Werror  
RM = rm -rf

SRC = main.cpp ./ConfigParser/ConfigParser.cpp ./ConfigParser/UtilsParser.cpp ./Server/Server.cpp
LIB =   ./ConfigParser/ConfigParser.hpp ./ConfigParser/UtilsParser.hpp ./Server/Server.hpp

OBJ = $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp $(LIB)
	$(CC) $(CFLAGS) -c $< -o $@

clean: 
	$(RM) $(OBJ)

fclean: clean
	$(RM) $(NAME)

re: fclean all