FROM alpine:3.24 AS build

RUN apk add --no-cache --update alpine-sdk linux-headers git zlib-dev openssl-dev gperf cmake

WORKDIR /root/DEV/telegram-rest-api

COPY CMakeLists.txt /root/DEV/telegram-rest-api
COPY td /root/DEV/telegram-rest-api/td
COPY telegram-bot-api /root/DEV/telegram-rest-api/telegram-bot-api
RUN mkdir -p build \
 && cd build \
 && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=.. .. \
 && cmake --build . --target install -j $(nproc) \
 && strip /root/DEV/telegram-rest-api/bin/telegram-bot-api

FROM alpine:3.24

ENV TELEGRAM_WORK_DIR="/var/lib/telegram-bot-api" \
    TELEGRAM_TEMP_DIR="/tmp/telegram-bot-api"

RUN apk add --no-cache --update openssl libstdc++
COPY --from=build /root/DEV/telegram-rest-api/bin/telegram-bot-api /usr/local/bin/telegram-bot-api
COPY docker-entrypoint.sh /docker-entrypoint.sh
RUN addgroup -g 101 -S telegram-bot-api \
 && adduser -S -D -H -u 101 -h ${TELEGRAM_WORK_DIR} -s /sbin/nologin -G telegram-bot-api -g telegram-bot-api telegram-bot-api \
 && chmod +x /docker-entrypoint.sh \
 && mkdir -p ${TELEGRAM_WORK_DIR} ${TELEGRAM_TEMP_DIR} \
 && chown telegram-bot-api:telegram-bot-api ${TELEGRAM_WORK_DIR} ${TELEGRAM_TEMP_DIR}\
 && chmod 700 ${TELEGRAM_TEMP_DIR}
USER telegram-bot-api:telegram-bot-api

# connect to 127.0.0.1 rather than localhost: the server listens on IPv4 only, while localhost
# resolves to ::1 first. busybox nc has no -z, so open a connection and close it immediately
HEALTHCHECK CMD nc -w 1 127.0.0.1 ${TELEGRAM_PORT:-8081} </dev/null
EXPOSE 8081/tcp 8082/tcp
ENTRYPOINT ["/docker-entrypoint.sh"]
