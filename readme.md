Chanserv (former #### One 21.1) - Jira and Trello CPP API and a Telegram Bot (++)\
Uses PostgreSQL as main DB and MS SQL QODBC3 as a backup DB       \
Buttons are saved in pgsql as well
\
\
Aug 2024    v.1\
Sep 2024    v.2\
Oct 2024    v.3\
Nov 2024    v.4\
Dec 2024    v.5\
Jan 2025    v.6\
Feb 2025    v.7\
Mar 2025    v.8\
Apr 2025    v.9\
May 2025    v.10\
Jun 2025    v.11\
Jul 2025    v.12\
Aug 2025    v.13\
Sep 2025    v.14\
Oct 2025    v.15\
Nov 2025    v.16\
Dec 2025    v.17\
Jan 2026    v.18\
Feb 2026    v.19\
Mar 2026    v.20\
Apr 2026    v.21\
May 2026    v.21.1
<br/>
- C++ 20
- CMake 3.28 (built using Qt Creator 14.0.2)
- Qt6 (built on 6.10.1)
- requires installed QTerminal package (except in docker container)
- boost
- curl
- openssl

\
Minimal Qt configuration:

configure -sql-odbc -sql-psql -skip qtwebengine -skip qtquick3d -skip qtmultimedia -skip qtgraphs -skip qtquick3dphysics -skip qtquickeffectmaker -skip qtspeech -skip qtdoc -skip qtquick -skip qtdeclarative -skip qtquickcontrols -skip qtquickcontrols2 -skip qtlocation -skip qtlottie -skip qtmqtt -skip qtopcua -skip qtquicktimeline -skip qtvirtualkeyboard -skip qtwebview -skip qttools -skip qttranslations -skip qt3d -skip qtcharts -skip qtcoap -skip qtdatavis3d -skip qtgrpc -skip qtremoteobjects -skip qtscxml -skip qtsensors -skip qtwayland -skip qtwebchannel

\
![x86](https://github.com/roosslan/tgbot_with_rest/blob/trunk/chanserv.gif?raw=true)
