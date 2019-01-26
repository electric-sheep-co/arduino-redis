#include "Redis.h"

/**
 * Check if the response is an error.
 * @return Return the error message or "".
 */
String Redis::checkError(String resp)
{
    if (resp.startsWith("-"))
    {
        return resp.substring(1, resp.indexOf("\r\n"));
    }
    return "";
}

/**
 * Open the Redis connection.
 */
RedisReturnValue Redis::connect(const char* password)
{
    if(conn.connect(addr, port)) 
    {
        // the NoDelay and Timeout should be specified prior to making the connection
        conn.setNoDelay(noDelay);
        conn.setTimeout(timeout);
        int passwordLength = strlen(password);
        if (passwordLength > 0)
        {
            conn.println("*2");
            conn.println("$4");
            conn.println("AUTH");
            conn.print("$");
            conn.println(passwordLength);
            conn.println(password);

	    int c = 0;
	    while (!conn.available() && c++ < 100) {
		    delay(10);
	    }
            String resp = conn.readStringUntil('\0');
            return resp.indexOf("+OK") == -1 ? RedisAuthFailure : RedisSuccess;
        }
        return RedisSuccess;
    }
    return RedisConnectFailure;
}

/**
 * Process the SET command (see https://redis.io/commands/set).
 * @param key The key.
 * @param value The assigned value.
 * @return If it's okay.
 */
bool Redis::set(const char* key, const char* value)
{
    conn.println("*3");
    conn.println("$3");
    conn.println("SET");
    conn.print("$");
    conn.println(strlen(key));
    conn.println(key);
    conn.print("$");
    conn.println(strlen(value));
    conn.println(value);

    String resp = conn.readStringUntil('\0');
    return resp.indexOf("+OK") != -1;
}

/**
 * Process the GET command (see https://redis.io/commands/get).
 * @param key The key.
 * @return Found value.
 */

String Redis::get(const char* key) 
{
    conn.println("*2");
    conn.println("$3");
    conn.println("GET");
    conn.print("$");
    conn.println(strlen(key));
    conn.println(key);

    String resp = conn.readStringUntil('\0');
    String error = checkError(resp);
    if (error != "")
    {
        return error;
    }
    if (resp.startsWith("$-1"))
    {
        return "";
    }
    int start = resp.indexOf("\r\n");
    int length = resp.substring(1, start).toInt();
    return resp.substring(start + 2, start + length + 2);
}

/**
 * Process the PUBLISH command (see https://redis.io/commands/publish).
 * @param key The channel.
 * @param value The message.
 * @return Number of subscribers which listen this message.
 */
int Redis::publish(const char* channel, const char* message)
{
    conn.println("*3");
    conn.println("$7");
    conn.println("PUBLISH");
    conn.print("$");
    conn.println(strlen(channel));
    conn.println(channel);
    conn.print("$");
    conn.println(strlen(message));
    conn.println(message);

    String resp = conn.readStringUntil('\0');
    if (checkError(resp) != "")
    {
        return -1;
    }
    return resp.substring(1, resp.indexOf("\r\n")).toInt();
}

/**
 * Close the Redis connection.
 */
void Redis::close(void)
{
    conn.stop();
}
