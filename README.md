# webserver.c

A simple webserver written in C. This server have dynamic routes, with `[id].c` files compiled into the shared objects, and hotreload features (with `hotreload.h`).

# How can I start?

You can run the <u>**build script**</u> (`./build.sh`) to compile the project. After compile the project you can run `./server.out` to start the server at the **PORT** 4242, then just open your browser and access `localhost:4242/something/42` and see the server running.

# How can I test the hot reload?

Just modify `handlers.c` or `request.c`, after that, use the build script to recompile the files and reload your browser.

## Example

You can try the following example:

1. Open `handlers.c`.
2. Go to `handle_request` function.
3. Modify the response char before the `send` function.
    - You can add the following before the `send` function.
        ```c
        strcat(response, "<b>HI!</b>");
        ```
4. Save the file and recompile the project.
5. Now refresh the localhost page and see the changes.

# How can I test the dynamic routing?

Try to access any variation of `localhost:4242/something/<id>`. The program will try to find a static file to render. If the file not exists, then `[id].so` will be used to gerate some of the content for the page.
