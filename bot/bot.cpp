#include <iostream>
#include <tgbot/tgbot.h>
#include <thread>
#include <chrono>
#include <fstream>
#include <mysql.h>
#include "QuaryException.h"

std::condition_variable cv_send_message;
std::condition_variable cv_timer;
std::mutex mtx;
std::string var;
std::string query;
std::string username;
char* user_id;

MYSQL* conn;
MYSQL_RES* res;
MYSQL_ROW row;

int access_lvl;
int key;
int key_quote = 1;
int coins;
static std::thread* thread_ptr = nullptr;

//bot of one toxic but funny person

void UserRegistration(TgBot::Message::Ptr message) {
    query = "SELECT COUNT(*) FROM `users` WHERE id = " + std::to_string(message->from->id);
    if (mysql_query(conn, query.c_str())) {
        throw QuaryException();
    }
    else {
        res = mysql_store_result(conn);
        row = mysql_fetch_row(res);
        if (row[0][0] == '0') {
            query = "INSERT INTO users(id, name) VALUES(" + std::to_string(message->from->id) + ", \"" + message->from->username + "\");";
            if (mysql_query(conn, query.c_str())) {
                throw QuaryException();
            }
        }
    }
}

std::string session() {
    SYSTEMTIME time;
    GetLocalTime(&time);

    int day = time.wDay, month = time.wMonth, dif, ho, mi, sec;
    std::cout << "how much time before session???" << std::endl;
    if ((month == 1 || month == 6) && day >= 11) {
        return u8"Сессия идет!";
        //std::cout << "SESSION IS GOING" << std::endl;
    }
    else {
        //while (true) {
        GetLocalTime(&time);
        if (time.wMonth > 6) {
            month = 13;
        }
        else {
            month = 6;
        }
        day = 11;
        dif = day - time.wDay;
        if (dif < 0) {
            month--;
            switch (time.wMonth) {
            case 1:
            case 3:
            case 5:
            case 7:
            case 8:
            case 10:
            case 12:
                dif = 42 - time.wDay;
                break;
            case 4:
            case 6:
            case 9:
            case 11:
                dif = 41 - time.wDay;
            default:
                dif = 39 - time.wDay + (time.wYear % 4);
            }
        }

        dif--;
        ho = 23 - time.wHour;
        mi = 59 - time.wMinute;
        sec = 59 - time.wSecond;
        return std::to_string(month - time.wMonth) + u8" Месяцев " + std::to_string(dif) + u8" Дней " + std::to_string(ho) + u8" Часов " + std::to_string(mi) + u8" Минут " + std::to_string(sec) + u8" Секунд";
        /*std::string res;
        std::cout << month - time.wMonth << " Months " << dif << " Days " << ho << " Hours " << mi << " Mins " << sec << " Seconds";*/
    }
}

void Timer_thread(int&& time) {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        printf("\ntimer while started");
        cv_timer.wait(lock, [=]() {return thread_ptr != nullptr; });
        printf("\ntimer notification accepted");
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(time));
        lock.lock();
        cv_send_message.notify_one();
        printf("\nsend_thread notified\n");
        lock.unlock();
        thread_ptr->join();
        lock.lock();
        delete thread_ptr;
        thread_ptr = nullptr;
        printf("\nsend_thread deleted");
    }
}

void Send_message_thread(long long& chat_id, int32_t messageId, const char* str, TgBot::Bot& bot) {
    long long ci = chat_id;
    int32_t mi = messageId;
    std::unique_lock<std::mutex> lock(mtx);
    printf("\nsend message thread started");
    cv_timer.notify_one();
    printf("\ntimer notified");
    cv_send_message.wait(lock);
    printf("\nwaiting notification accepted");
    bot.getApi().sendMessage(ci, str, false, mi);
}

void DelayMessage(long long& chat_id, int32_t messageId, const char* str, int&& time, TgBot::Bot& bot) {
    long long ci = chat_id;
    int32_t mi = messageId;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    mtx.lock();
    bot.getApi().sendMessage(ci, u8"шучу, не буду скидывать)))", false, mi);
    mtx.unlock();
}
//осторожно мАтЫ

int main() {
    TgBot::Bot bot("6749990719:AAFGCse6AjkFgsc10NFA8FdBPiXs3F7iiOM");

    //============================ КОМАНДЫ ============================//

    bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
        if (message->from->lastName != "") {
            var = u8"Ты " + message->from->firstName + " " + message->from->lastName + "?";
        }
        else {
            var = u8"Ты " + message->from->firstName + "?";
        }
        bot.getApi().sendMessage(message->chat->id, var);
        });


    bot.getEvents().onCommand("session", [&bot](TgBot::Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, session());
        });


    bot.getEvents().onCommand("quote", [&bot](TgBot::Message::Ptr message) {//присылает стикер с цитатой этого гения
        srand(time(0));
        try {
            UserRegistration(message);
        }
        catch (QuaryException& ex) {
            bot.getApi().sendMessage(message->chat->id, u8"не сейчас!!! я забыл кто ты такой!!!!");
            std::cout << ex.what() << std::endl;
            mysql_free_result(res);
            return 1;
        }
        query = "SELECT access_lvl_st FROM users WHERE id = " + std::to_string(message->from->id);
        if (mysql_query(conn, query.c_str())) {
            bot.getApi().sendMessage(message->chat->id, u8"не сейчас!");
            std::cerr << "\nquery failed: " << query << mysql_error(conn) << std::endl;
            return 1;
        }
        else {
            res = mysql_store_result(conn);
            row = mysql_fetch_row(res);
            access_lvl = std::stoi(row[0]);
        }
        if (access_lvl == 0) {
            auto message1 = bot.getApi().sendMessage(message->chat->id, u8"печатает...");
            auto message2 = bot.getApi().sendMessage(message->chat->id, u8"печатает...");
            auto message3 = bot.getApi().sendMessage(message->chat->id, u8"печатает...");
            auto message4 = bot.getApi().sendMessage(message->chat->id, u8"печатает...");
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            bot.getApi().editMessageText(u8"ааххахахахахахах", message->chat->id, message1->messageId);

            std::this_thread::sleep_for(std::chrono::seconds(1));
            bot.getApi().editMessageText(u8"чтобы получить цитату нужно заслужить мое доверие", message->chat->id, message2->messageId);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
            bot.getApi().editMessageText(u8"повышай доверие и получишь больше моих мудрых высказываний", message->chat->id, message3->messageId);

            std::this_thread::sleep_for(std::chrono::milliseconds(750));
            bot.getApi().editMessageText(u8"а пока гуляй", message->chat->id, message4->messageId);
            return 1;
        }
        key_quote = (rand() % access_lvl);
        switch (key_quote) {
        case 0:
            bot.getApi().sendSticker(message->chat->id, "CAACAgIAAxkBAAEs6z5mq2rJV3d26YFs_-kGXe61TXzuUQACwEUAAmgQ0UuE9rHoEfV4FjUE");
            break;
        case 1:
            bot.getApi().sendSticker(message->chat->id, "CAACAgIAAxkBAAEs61xmq24ZX_ULust3A5uWuWf1H8GhdAACtEEAAsVh0Us2TKGztcYUdjUE");
            break;
        case 2:
            bot.getApi().sendSticker(message->chat->id, "CAACAgIAAxkBAAEs61Bmq2y4lo9Wme-BBU96luAvRwv2QQACnTkAAnb0OUgcqcC-kXdWuDUE");
            break;
        case 3:
            bot.getApi().sendSticker(message->chat->id, "CAACAgIAAxkBAAEs7w5mrJd0yRTdsaRDGNwxgdNJF-OKrwACR00AAmnzaEkj6DZeMdFF1DUE");
            break;
        case 4:
            bot.getApi().sendSticker(message->chat->id, "CAACAgIAAxkBAAEs71hmrKt59nl4fRL5lirW34ypenuUywACk08AAmswYEnJf5G-zTQ_ijUE");
            break;
        case 5:
            bot.getApi().sendSticker(message->chat->id, "CAACAgIAAxkBAAEs71pmrKt7i6qWDRwZUH9p9aPLGT3_CAACxU0AAqZ0aUncDQWpkxSNnDUE");
            break;
        default:
            bot.getApi().sendMessage(message->chat->id, u8"успокойся уже да");
            break;
        }
        });

    bot.getEvents().onCommand("coin_holder", [&bot](TgBot::Message::Ptr message) {
        //регистрация пользователя
        try {
            UserRegistration(message);
        }
        catch (QuaryException& ex) {
            bot.getApi().sendMessage(message->chat->id, u8"у меня нет настроения кидать монетку в копилку(");
            std::cout << ex.what() << std::endl;
            mysql_free_result(res);
            return 1;
        }
        bot.getApi().sendPhoto(message->chat->id, "https://sun9-78.userapi.com/impg/Y02GK2lcTFPcAGHvcHJYkax6xoJbUVYp0B4NfQ/l4wn7wMv-KA.jpg?size=608x675&quality=95&sign=22887142739252955d65111ed1082042&type=album");
        query = "SELECT coins FROM `users` WHERE id = " + std::to_string(message->from->id);
        if (mysql_query(conn, query.c_str())) {
            bot.getApi().sendMessage(message->chat->id, u8"у меня нет настроения кидать монетку в копилку(");
            std::cerr << "\nquery failed: " << query << mysql_error(conn) << std::endl;
            return 1;
        }
        else {
            res = mysql_store_result(conn);
            row = mysql_fetch_row(res);
            coins = std::stoi(row[0]);
            coins++;
            query = "UPDATE users SET coins = " + std::to_string(coins) + " WHERE id = " + std::to_string(message->from->id);
            if (mysql_query(conn, query.c_str())) {
                std::cerr << "\nquery failed: " << query << mysql_error(conn) << std::endl;
                return 1;
            }
            else {
                std::cout << "coins added" << std::endl;
                bot.getApi().sendMessage(message->chat->id, u8"кинул монетку в копилку!");
            }
        }
        });

    bot.getEvents().onCommand("increase_trust", [&bot](TgBot::Message::Ptr message) {
        try {
            UserRegistration(message);
        }
        catch (QuaryException& ex) {
            bot.getApi().sendMessage(message->chat->id, u8"у меня месячные!!!! сегодня ничего не будет!!!!");
            std::cout << ex.what() << std::endl;
            mysql_free_result(res);
            return 1;
        }
        query = "SELECT coins, access_lvl_st FROM `users` WHERE id = " + std::to_string(message->from->id);
        if (mysql_query(conn, query.c_str())) {
            bot.getApi().sendMessage(message->chat->id, u8"мне не нужны твои деньги!!");
            std::cerr << "\nquery failed: " << query << mysql_error(conn) << std::endl;
            return 1;
        }
        else {
            res = mysql_store_result(conn);
            row = mysql_fetch_row(res);
            coins = std::stoi(row[0]);
            access_lvl = std::stoi(row[1]);
            int price = (access_lvl + 1) * 10; //первый уровень стоит 10, второй 20...
            if (coins >= price) {
                access_lvl++;
                coins -= price;
                query = "UPDATE users SET coins = " + std::to_string(coins) + ", access_lvl_st = " + std::to_string(access_lvl) + " WHERE id = 542852695";
                if (mysql_query(conn, query.c_str())) {
                    bot.getApi().sendMessage(message->chat->id, u8"мне не нужны твои деньги!!");
                    std::cerr << "\nquery failed: " << query << mysql_error(conn) << std::endl;
                    return 1;
                }
                //потом при каждом новом уровне добавлять разные истории про Дмитрия, которые объясняют полученную цитату - мне кажется крутейшая атмосферная идея
                bot.getApi().sendMessage(message->chat->id, u8"так уж и быть, вижу ты нормальный тип!");
            }
            else {
                auto message1 = bot.getApi().sendMessage(message->chat->id, u8"печатает...");
                auto message2 = bot.getApi().sendMessage(message->chat->id, u8"печатает...");
                auto message3 = bot.getApi().sendMessage(message->chat->id, u8"печатает...");

                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                bot.getApi().editMessageText(u8"ааххахахахахахах", message->chat->id, message1->messageId);

                std::this_thread::sleep_for(std::chrono::seconds(1));
                bot.getApi().editMessageText(u8"чтобы заслужить мое доверие нужно подкопить денег!!!!!!!!!!!!", message->chat->id, message2->messageId);

                std::this_thread::sleep_for(std::chrono::milliseconds(1500));
                bot.getApi().editMessageText(u8"ТАПНИ ДМИТРИЯ!!!!!!!!!!!!!", message->chat->id, message3->messageId);
            }
        }
        });

    bot.getEvents().onCommand("your_trust", [&bot](TgBot::Message::Ptr message) {
        try {
            UserRegistration(message);
        }
        catch (QuaryException& ex) {
            bot.getApi().sendMessage(message->chat->id, u8"ты кто");
            std::cout << ex.what() << std::endl;
            mysql_free_result(res);
            return 1;
        }
        query = "SELECT access_lvl_st FROM `users` WHERE id = " + std::to_string(message->from->id);
        if (mysql_query(conn, query.c_str())) {
            bot.getApi().sendMessage(message->chat->id, u8"не знаю!");
            std::cerr << "\nquery failed: " << query << mysql_error(conn) << std::endl;
            return 1;
        }
        res = mysql_store_result(conn);
        row = mysql_fetch_row(res);
        access_lvl = std::stoi(row[0]);
        switch (access_lvl) {
        case 0:
            bot.getApi().sendMessage(message->chat->id, u8" ваш уровень - НОЛЬ!!! вы - вафель (петух)");
            break;
        case 1:
            bot.getApi().sendMessage(message->chat->id, u8"ваш уровень - ОДИН!!! вы - чушка");
            break;
        case 2:
            bot.getApi().sendMessage(message->chat->id, u8"ваш уровень - ДВА!!! вы - пацан");
            break;
        case 3:
            bot.getApi().sendMessage(message->chat->id, u8"ваш уровень - ТРИ!!! вы - мужик");
            break;
        case 4:
            bot.getApi().sendMessage(message->chat->id, u8"ваш уровень - ЧЕТЫРЕ!!! вы - блатной");
            break;
        case 5:
            bot.getApi().sendMessage(message->chat->id, u8"ваш уровень - ПЯТЬ!!! вы - пахан");
            break;
        case 6:
            bot.getApi().sendMessage(message->chat->id, u8"привет Яна пойдем гулять");
            bot.getApi().sendSticker(message->chat->id, "CAACAgIAAxkBAAEteMhm1N2uA1VrJzIeUihwnL3eUbRm0wAC5A0AAvz70UjzPFahRIdPHjUE");
            break;
        default:
            bot.getApi().sendMessage(message->chat->id, u8"успокойся уже да");
            break;
        }


        });

    //============================ ЛЮБОЕ СООБЩЕНИЕ ============================//

    bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message) {
        printf("\nUser wrote %s\n", message->text.c_str());
        if (StringTools::startsWith(message->text, u8"евген") || StringTools::startsWith(message->text, u8"Евген")) {
            bot.getApi().sendMessage(message->chat->id, u8"Евген привет братуха))", false, message->messageId);
            return;
        }
        if (message->text.size() > 4096) {
            bot.getApi().deleteMessage(message->chat->id, message->messageId);
            bot.getApi().sendMessage(message->chat->id, u8"да ты че ахуел блять говно я не буду на это отвечать тебя бы за такое на зоне бы выебали шпана ебаная");
            return;
        }
        if (StringTools::startsWith(message->text, "/coin_holder")) {
            return;
        }
        if (StringTools::startsWith(message->text, "/start")) {
            return;
        }
        if (StringTools::startsWith(message->text, "/session")) {
            return;
        }
        if (StringTools::startsWith(message->text, "/quote")) {
            return;
        }

        //============================ СООБЩЕНИЕ ТОЛЬКО МАКАРУ ============================//

        if (message->from->username == "microsoft2012") { //alexschetin1621 microsoft2012
            srand(time(0));
            key = (rand() % 8) + 1;
            //key = 8;
            switch (key) {
            case 1:bot.getApi().sendMessage(message->chat->id, u8"о макар помоги рк решить");
                break;
            case 2: bot.getApi().sendMessage(message->chat->id, u8"помоги");
                break;
            case 3: bot.getApi().sendMessage(message->chat->id, u8"здарова макар помогай");
                break;
            case 4: bot.getApi().sendMessage(message->chat->id, u8"макар мне нужно дз решить сделай");
                break;
            case 5: bot.getApi().sendMessage(message->chat->id, u8"в смысле " + message->text + u8" макар помоги хоть с микроконтрольной!");
                break;
            case 6: bot.getApi().sendMessage(message->chat->id, u8"макар ну ты же мой друг помоги");
                break;
            case 7: bot.getApi().sendMessage(message->chat->id, u8"макар)) готов решать рк?)))");
                break;
            case 8: {
                bot.getApi().sendMessage(message->chat->id, u8"ну хочешь я тебе денег скину? помоги тебе же не сложно!");

                //сообщение снизу отправится через время отдельным потоком, то есть пока бот ждет, чтобы отправить "шучу не буду скидывать", он также 
                //обрабатывает другие сообщения

                //создать поток с таймером, который находится вне всяких свитчкейсов и тд, и поток с отправкой сообщения, внутри свитчкейса, который сначала запускает поток с таймером
                //с помощью кондишен вариабле, а когда таймер закончится, поток с таймером уведомляет с помощью другого кондитион вариабле поток с отправкой сообщения, 
                //чтобы он отправил сообщение. не эффективно но мне нравится реализовать такую связь и поработать с мьютексами

                if (thread_ptr == nullptr) {
                    thread_ptr = new std::thread(Send_message_thread, std::ref(message->chat->id), std::ref(message->messageId), u8"шучу, не буду скидывать)))", std::ref(bot));
                }

                /*ниже тут более эффективное но скучное решение той же задачи

                if (thread_ptr != nullptr) {
                    thread_ptr->detach();
                    delete thread_ptr;
                    thread_ptr = nullptr;
                }

                thread_ptr = new std::thread(DelayMessage, std::ref(message->chat->id), std::ref(message->messageId), u8"шучу, не буду скидывать)))", 5, std::ref(bot));
                */
                break;
            }
            case 9: bot.getApi().sendMessage(message->chat->id, u8"Что!? " + message->text + u8"?\nНадеюсь, ты, сын шлюхи, хоть в этот раз за свои слова постоишь. Мне вот интересно, у тебя отец такое же трепло как и ты?");
                break;
            default: bot.getApi().sendMessage(message->chat->id, u8"этого ответа ты никогда не должен был получить, напиши моему создателю");
            }
            return;
        }

        //=================================================================================//
        bot.getApi().sendMessage(message->chat->id, u8"Что!? " + message->text + u8"?\nНадеюсь, ты, сын шлюхи, хоть в этот раз за свои слова постоишь. Мне вот интересно, у тебя отец такое же трепло как и ты?");
        //одна из его цитат нашему одногрупу

        });
    try {
        conn = mysql_init(0);
        conn = mysql_real_connect(conn, "127.0.0.1", "root", "", "tgbot", 3306, NULL, 0);

        if (conn) {
            printf("Connected to database\n");
            printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
            std::thread th(Timer_thread, 5);
            TgBot::TgLongPoll longPoll(bot);
            while (true) {
                printf("\nLong poll started");
                longPoll.start();
            }
        }
        else {
            std::cerr << "Connection to database has failed: " << mysql_error(conn) << std::endl;
            mysql_close(conn);
            return 1;
        }
    }
    catch (TgBot::TgException& e) {
        printf("error: %s\n", e.what());
    }
    return 0;
}