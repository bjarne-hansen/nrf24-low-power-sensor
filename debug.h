
#ifdef DEBUG_PRINT
#ifdef __PRINTF_H__
#define debug_begin_nrf24()       printf_begin()
#endif
#define debug_begin()             Serial.begin(9600); \
                                  debug_begin_nrf24()
#define debug_print(args...)      Serial.print(args)
#define debug_println(args...)    Serial.println(args)
#ifdef __PRINTF_H__
#define debug_nrf24(radio)        radio.printDetails()
#endif
#ifdef _Time_h
#define debug_date(t)             debug_print(year(t)); \
                                  debug_print("-"); \
                                  debug_print(month(t) < 10 ? "0" : ""); \
                                  debug_print(month(t)); \
                                  debug_print("-"); \
                                  debug_print(day(t) < 10 ? "0" : ""); \
                                  debug_print(day(t))
#define debug_time(t)             debug_print(hour(t) < 10 ? "0" : ""); \
                                  debug_print(hour(t)); \
                                  debug_print(':'); \
                                  debug_print(minute(t) < 10 ? "0" : ""); \
                                  debug_print(minute(t)); \
                                  debug_print(':'); \
                                  debug_print(second(t) < 10 ? "0" : ""); \
                                  debug_print(second(t))
#define debug_datetime(t)         debug_date(t); debug_print(' '); debug_time(t)      
#define debug_delay(ms)           delay(ms)                       
#endif
#else
#define debug_begin_nrf24()
#define debug_begin()     
#define debug_print(args...)    
#define debug_println(args...)  
#define debug_nrf24(radio)
#define debug_date(t)
#define debug_time(t)
#define debug_datetime(t)
#define debug_delay(ms)
#endif
