FROM alpine:3.12
# install gcc, make, libc and the bdw gc
RUN apk add --no-cache gcc make libc-dev gc-dev
# install leg
RUN wget -qO- https://www.piumarta.com/software/peg/peg-0.1.18.tar.gz | tar xz \
    && make -C /peg-0.1.18 leg >/dev/null 2>&1 \
    && mv /peg-0.1.18/leg /usr/bin/ \
    && rm -r peg-0.1.18
# Add project files and compile
WORKDIR /javascrypt
COPY . .
RUN make