#include <stdio.h>
#include <iostream>
#include <tgbot/tgbot.h>
#include <thread>
#include <chrono>

std::condition_variable cv_send_message;
std::condition_variable cv_timer;
std::mutex mtx;
std::mutex mtx2;
std::string var;
std::future<std::string> rep;
int key;
bool special = 0;
static std::thread* thread_ptr = nullptr;
static std::thread* thread_timer_ptr = nullptr;

//bot of one toxic but funny person

std::string session() {
    SYSTEMTIME time;
    GetLocalTime(&time);

    int day = time.wDay, month = time.wMonth, dif, ho, mi, sec;
    std::cout << "how much time before session???" << std::endl;
    if ((month == 1  || month == 6) && day >= 11) {
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

//осторожно мАтЫ

int main() {
    TgBot::Bot bot("не скажу)");

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
    bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message) {
        printf("\nUser wrote %s\n", message->text.c_str());
        if (StringTools::startsWith(message->text, u8"евген") || StringTools::startsWith(message->text, u8"Евген")) {
            bot.getApi().sendMessage(message->chat->id, u8"Евген привет братуха))",false, message->messageId);
            return;
        }
        if (message->text.size() > 4096) {
            bot.getApi().deleteMessage(message->chat->id, message->messageId);
            bot.getApi().sendMessage(message->chat->id, u8"да ты че ахуел блять говно я не буду на это отвечать тебя бы за такое на зоне бы выебали шпана ебаная");
            return;
        }
        if (StringTools::startsWith(message->text, "/start")) {
            return;
        }
        if (StringTools::startsWith(message->text, "/session")) {
            return;
        }
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

                break;
            }
            default: bot.getApi().sendMessage(message->chat->id, u8"этого ответа ты никогда не должен был получить напиши моему создателю");
            }
            return;
        }
        
        bot.getApi().sendMessage(message->chat->id, u8"Что!? " + message->text + u8"?\nНадеюсь, ты, сын шлюхи, хоть в этот раз за свои слова постоишь. Мне вот интересно, у тебя отец такое же трепло как и ты?");
        //одна из его цитат нашему одногруппнику
        });
    try {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        std::thread th (Timer_thread, 5);
        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            printf("\nLong poll started");
            longPoll.start();
        }
    }
    catch (TgBot::TgException& e) {
        printf("error: %s\n", e.what());
    }
    return 0;
}