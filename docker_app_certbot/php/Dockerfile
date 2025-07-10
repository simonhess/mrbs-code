FROM php:7.3-apache

ARG DOMAIN

RUN a2enmod rewrite
RUN apt-get update && apt-get install -y locales-all cron certbot python3-certbot-apache
RUN if [ -d "/etc/letsencrypt/live/${DOMAIN}" ]; then \ 
certbot --apache -d "${DOMAIN}" -n;\
fi
RUN docker-php-ext-install mysqli pdo pdo_mysql