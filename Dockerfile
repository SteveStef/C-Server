# Use an official gcc runtime as a parent image
FROM gcc:latest

WORKDIR .

# Copy the current directory contents into the container at /usr/src/myapp
COPY . .

# Compile the server code
RUN gcc -o server main.c

EXPOSE 9000

# Run server.c when the container launches
CMD ["./server"]
