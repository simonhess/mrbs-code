FROM php:8.3-apache

ARG DOMAIN

RUN a2enmod rewrite
COPY docker_app_certbot/letsencrypt /etc/letsencrypt
RUN apt-get update && apt-get install -y locales-all libicu72 libicu-dev cron certbot 
RUN apt-get install -y python3-certbot-apache
RUN if [ -d "/etc/letsencrypt/live/${DOMAIN}" ]; then \ 
certbot --apache -d "${DOMAIN}" -n;\
fi
RUN docker-php-ext-install mysqli pdo pdo_mysql intl