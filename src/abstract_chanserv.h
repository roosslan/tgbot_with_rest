#pragma once

#include "support.h"
#include "structures.h"
#include "chanconf.h"

#include <qcoreapplication.h>
#include <qhostinfo.h>
#include <qprocess.h>
#include <qstring.h>
#include <qdir.h>

class AbstractChanserv : public ChanConf
{
public:    
    AbstractChanserv();

    // Запрещаем и копирование и перемещение базового класса (неявно)
    AbstractChanserv(const AbstractChanserv&) = delete;
    AbstractChanserv& operator=(const AbstractChanserv&) = delete;
    // Запрещаем операции перемещения (явно)
    AbstractChanserv(AbstractChanserv&&) = delete;
    AbstractChanserv& operator=(AbstractChanserv&&) = delete;

    std::deque<std::string> sticker_list;
    std::vector<contractor> v_contractors;
    std::vector<long> v_allowed_contractors;
    std::vector<long> v_staff;
    std::deque<long> v_consumers;

    /*
     *   Антиспам/анти DDOS на будущее
     *   std::vector<std::string> allowedConsumers;
     */

    int LISTCHECK_DURATION_IN_MSEC = 30000;    

    int get_mem_size(const std::string& script_path);
    void reload_consumers_list();
    void reload_contractors_list();
};
