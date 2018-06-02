#include "Redis.h"

/**
 * @param addr Redis address.
 * @param port Redis port.
 */
Redis::Redis(const char *addr, int port)
{
    this->addr = addr;
    this->port = port;
    this->NoDelay = false;
    this->Timeout = 100;
}

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
 * Open the Redis connection with no password.
 * @return If it's okay.
 */
bool Redis::begin(void)
{
    return this->begin("");
}

/**
 * Open the Redis connection.
 * @param password Redis password ("" if there is no password).
 * @return If it's okay.
 */
bool Redis::begin(const char *password)
{
    if(this->conn.connect(this->addr, this->port)) 
    {
        // the NoDelay and Timeout should be specified prior to making the connection
        this->conn.setNoDelay(this->NoDelay);
        this->conn.setTimeout(this->Timeout);
        int passwordLength = strlen(password);
        if (passwordLength > 0)
        {
            this->conn.println("*2");
            this->conn.println("$4");
            this->conn.println("AUTH");
            this->conn.print("$");
            this->conn.println(passwordLength);
            this->conn.println(password);

            String resp = this->conn.readStringUntil('\0');
            return resp.indexOf("+OK") != -1;
        }
        return true;
    }
    return false;
}

/**
 * Set the noDelay option for the ESP8266 WiFi Client
 * @param val true or false
 * @return If it's okay.
 */
bool Redis::setNoDelay(bool val)
{
    if (this->conn.connected())
    {
        // It should be specified prior to making the connection
        return false;
    }
    else
    {
        this->NoDelay = val;
        return true;
    }
}

/**
 * Set the Timeout option for the ESP8266 WiFi Client
 * @param val timeout duration in milliseconds (long)
 * @return If it's okay.
 */
bool Redis::setTimeout(long val)
{
    if (this->conn.connected())
    {
        // It should be specified prior to making the connection
        return false;
    }
    else
    {
        this->Timeout = val;
        return true;
    }
}

/**
 * Process the SET command (see https://redis.io/commands/set).
 * @param key The key.
 * @param value The assigned value.
 * @return If it's okay.
 */
bool Redis::set(const char *key, const char *value)
{
    this->conn.println("*3");
    this->conn.println("$3");
    this->conn.println("SET");
    this->conn.print("$");
    this->conn.println(strlen(key));
    this->conn.println(key);
    this->conn.print("$");
    this->conn.println(strlen(value));
    this->conn.println(value);

    String resp = this->conn.readStringUntil('\0');
    return resp.indexOf("+OK") != -1;
}

/**
 * Process the GET command (see https://redis.io/commands/get).
 * @param key The key.
 * @return Found value.
 */

String Redis::get(const char *key) 
{
    this->conn.println("*2");
    this->conn.println("$3");
    this->conn.println("GET");
    this->conn.print("$");
    this->conn.println(strlen(key));
    this->conn.println(key);

    String resp = this->conn.readStringUntil('\0');
    String error = this->checkError(resp);
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
int Redis::publish(const char *channel, const char *message)
{
    this->conn.println("*3");
    this->conn.println("$7");
    this->conn.println("PUBLISH");
    this->conn.print("$");
    this->conn.println(strlen(channel));
    this->conn.println(channel);
    this->conn.print("$");
    this->conn.println(strlen(message));
    this->conn.println(message);

    String resp = this->conn.readStringUntil('\0');
    if (this->checkError(resp) != "")
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
    this->conn.stop();
}
