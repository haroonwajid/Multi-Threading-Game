CXX = g++
CXXFLAGS = -Wall -pthread -std=c++11 -I/opt/homebrew/Cellar/sdl2/2.30.9/include
LDFLAGS = -pthread -L/opt/homebrew/Cellar/sdl2/2.30.9/lib -lSDL2

TARGET = game
SRCS = game.cpp
OBJS = $(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)