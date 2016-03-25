#What is this

This is my graduation design at Hust.  
I want to write a client server architecture computing program. which client send request, and then server execute computing program and send result back   

#Bsic idea

1. Using proxy worker model, proxy accept request and select a worker, using some load balancing method, and forward that request.
2. Worker fork a child, let he execute the computing program on that recieved data, hint: design the computing program as a .so.
3. Using basic C socket to communicate between client and proxy, proxy and worker. data file sent is small, less than 10M, typically 100KB.
4. load balancing is direved from apache httpd, called [mod_lbmethod_byrequests](https://httpd.apache.org/docs/current/mod/mod_lbmethod_byrequests.html).
