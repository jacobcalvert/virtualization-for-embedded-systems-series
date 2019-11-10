##########################################
# File: Dockerfile
# Author: Jacob Calvert <jcalvert@jacobncalvert.com>
# Date: Nov-09-2019
# 
# This Dockerfile creates a deployment environment for 
# the sample application
#
##########################################

# start from the basic ubuntu image
FROM ubuntu

# create a app/bin directory in our container
RUN mkdir -p /app/bin 

# copy in the app and a startup script
COPY modem /app/bin
COPY start-modem.sh /

# set the entry point
ENTRYPOINT /start-modem.sh

