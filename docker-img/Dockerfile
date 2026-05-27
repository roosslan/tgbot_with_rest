# The chanserv build file
FROM carlonluca/qt-dev:6.10.0

ENTRYPOINT ["python3"]
# ENTRYPOINT ["/bin/bash"]

RUN mkdir -p /var/log/supervisor
RUN mkdir -p /home/rasa/chanserv/listchecker

COPY listchecker/listchecker_mem_used.sh /home/rasa/chanserv/listchecker/listchecker_mem_used.sh
COPY chanserv_mem_used.sh /home/rasa/chanserv/chanserv_mem_used.sh
COPY sendmail.ps1 /home/rasa/sendmail.ps1
COPY listchecker.sh /home/rasa/chanserv/listchecker.sh
COPY reboot.sh /home/rasa/chanserv/reboot.sh
COPY supervisord.conf /etc/supervisor/conf.d/supervisord.conf
COPY libqsqlpsql.so /opt/Qt-amd64-6.10.0/plugins/sqldrivers
COPY libqsqlodbc.so /opt/Qt-amd64-6.10.0/plugins/sqldrivers
COPY manapihttp-v1.0.0ubuntu24.04-dbg.deb /root
COPY msodbcsql18_18.5.1.1-1_amd64.deb /root
COPY mssql-tools18_18.4.1.1-1_amd64.deb /root

# RUN chmod +x /root/restart_bot.sh 

RUN apt-get update && mkdir /root/logs && \
echo 'Binary::apt::APT::Keep-Downloaded-Packages "1";' | tee /etc/apt/apt.conf.d/99-keep-downloads 

RUN apt install -y libboost-all-dev libcpprest-dev libcjson-dev odbcinst unixodbc unixodbc-dev postgresql-contrib libpq-dev postgresql-common libuv1-dev mc libgmp-dev libnghttp2-dev net-tools wget apt-transport-https software-properties-common snap python3 supervisor

RUN dpkg -i /root/manapihttp-v1.0.0ubuntu24.04-dbg.deb && \

ACCEPT_EULA=Y dpkg -i /root/msodbcsql18_18.5.1.1-1_amd64.deb && \
ACCEPT_EULA=Y dpkg -i /root/mssql-tools18_18.4.1.1-1_amd64.deb

EXPOSE 53 179 8080 

WORKDIR /root
# Соответственно папка будет /home/rasa/chanserv

ENV LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:/opt/Qt-amd64-6.10.0/lib"
ENV PATH="${PATH}:/opt/Qt-amd64-6.10.0/bin:/opt/Qt-amd64-6.10.0/libexec:/opt/mssql-tools18/bin"
ENV QT_HOST_PATH="/opt/Qt-amd64-6.10.0/"
ENV QT_PLUGIN_PATH="/opt/Qt-amd64-6.10.0/plugins"
#ENV CHANSERV_CONFIG=/root/chanserv.ini
#ENV CHANSERV_LOGDIR=/root/logs

# Одновременный запуск нескольких процессов в docker'е
CMD ["/usr/bin/supervisord"]

