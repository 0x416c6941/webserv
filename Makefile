NAME = webserv

CC = c++
FLAGS = -Wall -Wextra -Wsign-conversion -pedantic -Werror -Wreorder -std=c++98	\
	-g -fsanitize=address

SRC_DIR  = src
INC_DIR  = include

SRC_FILES = main.cpp				\
			ConfigFile.cpp		\
			ConfigParser.cpp	\
			ServerConfig.cpp	\
			ServerBuilder.cpp	\
			Location.cpp		\
			ServerManager.cpp	\
			ClientConnection.cpp	\
			HTTPRequest.cpp		\
			HTTPResponse.cpp	\
			errors.cpp		\
			utils.cpp		\
			debug.cpp		\


SRC_FILES := $(addprefix $(SRC_DIR)/,$(SRC_FILES))

# Object files.
OBJ_DIR = obj
OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

# Default target
all: $(NAME)

# Link object files into the final executable
$(NAME): $(OBJ_FILES)
	$(CC) $(FLAGS) -I$(INC_DIR) -o $@ $^

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CC) $(FLAGS) -I$(INC_DIR) -c $< -o $@

# Clean object files and binary
clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
