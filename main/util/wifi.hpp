
#pragma once



// FORWARD DECLARATIONS =====================================================================================

namespace APP::wifi {

    // CONSTANTS ============================================================================================

    // MACROS ===============================================================================================

    // TYPES ================================================================================================
    
    struct Credential {

        const char*                 ssid;
        const char*                 password;
    };

    // STATIC VARIABLES =====================================================================================

    // FUNCTION DECLARATION =================================================================================

    // Call once at startup. Must happen before any other function.
    void init();


    // Replace the stored network list with the given credentials.
    void set_credentials(const std::vector<Credential>& creds);
    
    
    // Convenience overload for braced initializer lists.
    void set_credentials(std::initializer_list<Credential> creds);


    // @brief  Try to connect to one of the stored networks, sync time via SNTP, and disconnect when done.
    // @return true if time was successfully synchronized.
    bool sync_time();


    // Disconnect and stop the Wi‑Fi driver completely (saves power).
    void disconnect();


    // Start a timer that calls sync_time() every interval_ms milliseconds.
    void start_periodic_sync(u32 interval_ms);


    // Stop the periodic sync timer (if running).
    void stop_periodic_sync();


    // stop the startup retry timer
    void stop_retry_sync();


    // Start a retry loop: try every retry_interval_ms until time sync succeeds,
    // then automatically switch to long_term_interval_ms.
    void start_retry_sync_until_success(u32 retry_interval_ms, u32 long_term_interval_ms);

    // TEMPLATE DECLARATION =================================================================================

    // CLASS DECLARATION ====================================================================================

}
