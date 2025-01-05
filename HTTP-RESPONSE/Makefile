NAME = CGI_TESTING
SRC =  HttpRequest.cpp \
	   Cgi.cpp \

OBJ = $(SRC:.cpp=.o)
CXX = c++
CXXFLAGS = -Wall -Werror -Wextra #-std=c++98
all : $(NAME)

$(NAME) : $(OBJ)
	$(CXX) $(SRC) $(CXXFLAGS) -o $(NAME)
clean :
	rm -f $(OBJ)
fclean : clean
	rm -f $(NAME)
re : fclean all
.PHONY : all clean fclean re
.SECONDARY : $(OBJ)