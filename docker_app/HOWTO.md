# Dev environment 

This docker setup is supposed to get a running system for local development.

## Run locally

* Compose Docker containers:
  ~~~
  cd docker_app 
  docker-compose up -d
  ~~~
  (Linux users may need `sudo` or `docker login`)

* Open [localhost:8080](http://localhost:8080) in your browser \
  Logins are defined at first use. (There may be example data in the future)

## Additonal information

### Inspect database
Open [localhost:8888](http://localhost:8888) in your browser for phpmyadmin \
Login: `mrbs:mrbs`

Alternatively, you can connect to the database using the command like tool `mysql`:
~~~
docker-compose db mysql -u mrbs -pmrbs mrbs
~~~

### View logs

View apache webserver logs:
~~~
docker-compose logs www
~~~
View database logs:
~~~
docker-compose logs db
~~~


### Live reloading
The repository's source code is mounted into the docker containers. That means, changes of code take effect immediately after refreshing the browser.

However, when configuration of php and database is change, you have to reset the containers.

### Reset containers

* Stop Docker containers:
  ~~~
  docker-compose down
  ~~~
* Delete persisted volumes:
  ~~~
  docker volume prune
  ~~~
* Rebuild container images:
  ~~~
  docker-compose build
  ~~~

### Run certbot in apache server

Change domain name in .env file to your domain:

DOMAIN="DOMAIN"

Copy your letsencrypt folder with valid certificates to this folder

### Configure mrbs and e-mail notifications

Set the following global variables to your settings:

$mrbs_company = "";

$mail_settings['from'] = 'admin_email@your.org';

$smtp_settings['host'] = '';  // SMTP server
$smtp_settings['port'] = 465;
$smtp_settings['auth'] = true;
$smtp_settings['secure'] = 'ssl';
$smtp_settings['username'] = '';       // Username (if using authentication)
$smtp_settings['password'] = '';

### Configure database

Add users to the database by appending lines to the tables.my.sql file:

INSERT INTO mrbs_users (id, name, password_hash, level) VALUES (0,'newuser', MD5('password'), 2);

