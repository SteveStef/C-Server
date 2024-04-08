# Use an official gcc runtime as a parent image
FROM gcc:latest

WORKDIR .

# Copy the current directory contents into the container at /usr/src/myapp
COPY . .

# Compile the server code
RUN gcc -o server server.c

# Make port 8080 available to the world outside this container
EXPOSE 8080

# Run server.c when the container launches
CMD ["./server"]
