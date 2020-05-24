//  This is the source code based on example 3 used in Chapter 2
//  "The Basics" of the omniORB user guide.
//
//  This is the client. It uses the COSS naming service
//  to obtain the object reference.
//
//  You need omniNames service running and omniORB.cfg configured
//  (try "InitRef = SimpleDemo=corbaname::localhost" for local tests)
//  as well as omniORB libraries installed in your system.
//
//  Usage:
//
//
//        On startup, the client lookup the object reference from the
//        COS naming service.
//
//        The name which the object is bound to is as follows:
//              root  [context]
//               |
//              text  [context] kind [my_context]
//               |
//              Echo  [object]  kind [Object]
//
#include <echo.hh>

#include <chrono>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#ifdef _WIN32
#   include <conio.h>
#else
#   include <termios.h>
#   include <unistd.h>
#endif  // def _WIN32

static CORBA::Object_ptr getObjectReference(CORBA::ORB_ptr orb);

static void sendEchoString(Echo_ptr e, std::string &str)
{
    if (CORBA::is_nil(e)) {
        std::cerr << "Echo: The object reference is nil!\n";
        return;
    }

    CORBA::String_var src = str.c_str();
    CORBA::String_var dest = e->echoString(src);
}

#ifndef _WIN32      //POSIX related unbuffered input

enum class PosixMode
{
    DEFAULT,
    UNBUFFERED,

    MAX_MODES
};

void setPosixMode(PosixMode pm)
{
    static struct termios previous, current;
    if (pm == PosixMode::DEFAULT) {
        tcsetattr(STDIN_FILENO, TCSANOW, &previous);
        return;
    }
    if (pm == PosixMode::UNBUFFERED) {
        tcgetattr(STDIN_FILENO, &previous);
        current = previous;
        current.c_lflag &= ~(ICANON);
        tcsetattr(STDIN_FILENO, TCSANOW, &current);
        return;
    }
    return;
}

enum class PosixTimeout
{
    ENABLE,
    DISABLE
};

int getPosixKey(PosixTimeout pt)
{
    int c {0};
    struct timeval tv;
    fd_set fs;
    tv.tv_usec = tv.tv_sec = 0;

    FD_ZERO(&fs);
    FD_SET(STDIN_FILENO, &fs);

    select(STDIN_FILENO + 1, &fs, 0, 0, (pt == PosixTimeout::DISABLE) ? 0 : &tv);
    if (FD_ISSET(STDIN_FILENO, &fs)) {
        c = getchar();
    }
    return c;
}
#endif  // ndef _WIN32

int getKey()
{
    int c {0};
#ifdef _WIN32
    c = _getch();
    if (c == 13)
        std::cout << '\n';
    else
        std::cout << static_cast<char>(c);
#else
    setPosixMode(PosixMode::UNBUFFERED);
    c = getPosixKey(PosixTimeout::DISABLE);
    setPosixMode(PosixMode::DEFAULT);
#endif  // def _WIN32
    return c;
}

////////////////////////////////////////////////////////////////////////////////

int main (int argc, char **argv)
{
    try {
        CORBA::ORB_var orb = CORBA::ORB_init(argc, argv);

        CORBA::Object_var obj = getObjectReference(orb);

        Echo_var echoRef = Echo::_narrow(obj);

        std::condition_variable cv;
        std::mutex mutex;
        std::deque<std::string> symbols{}; // protected by mutex

        // thread to read from console
        std::thread io{[&]{

            std::cerr << "New session initialized.\n";
            symbols.emplace_back("\nNew session initialized.\n");

            int tmp{};
            std::string str{};

            while (true) {
                tmp = getKey();
                if ((tmp >= 32 && tmp <=126) || (tmp == 10) || (tmp == 13)){
                    if (tmp == 13)
                        str = '\n';
                    else
                        str = static_cast<char>(tmp);
                    symbols.emplace_back(str);
                }
                else{
                    std::cerr << "\nERROR: Unprintable charachter detected. "
                              << "\nInput is ignored. Press ENTER to continue...";
                    symbols.emplace_back("\nERROR: Unprintable charachter detected.\n");
                    std::cin.clear();
                    for (int count{0}; ; ++count){
                        tmp=getKey();
                        if (tmp == 10 || tmp == 13) break;
                        if (count >= 9){
                            std::cerr << "\nInput is ignored. Press ENTER to continue...";
                            count = -1;
                        }
                    }
                    std::cerr << "Session resumed.\n";
                    symbols.emplace_back("Session resumed.\n");

                }
                std::lock_guard lock{mutex};
                cv.notify_one();
            }
        }};

        // the nonblocking thread
        std::deque<std::string> toProcess;
        while (true) {
            {
                // critical section
                std::unique_lock lock{mutex};
                if (cv.wait_for(lock, std::chrono::seconds(0), [&]{ return !symbols.empty(); })) {
                    // get a new batch of symbols to process
                    std::swap(symbols, toProcess);
                }
            }
            if (!toProcess.empty()) {
                for (auto&& symbol : toProcess) {
                    // process symbols received by io thread
                    sendEchoString(echoRef, symbol);
                }
                toProcess.clear();
            }
            //std::cerr << "waiting...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        orb->destroy();
    }
    catch (CORBA::TRANSIENT&) {
        std::cerr << "Caught system exception TRANSIENT -- unable to contact the "
                  << "server.\n";
    }
    catch (CORBA::SystemException& ex) {
        std::cerr << "Caught a CORBA::" << ex._name() << '\n';
    }
    catch (CORBA::Exception& ex) {
        std::cerr << "Caught CORBA::Exception: " << ex._name() << '\n';
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////

static CORBA::Object_ptr getObjectReference(CORBA::ORB_ptr orb)
{
    CosNaming::NamingContext_var rootContext;

    try {
        // Obtain a reference to the root context of the Name service:
        CORBA::Object_var obj;
        obj = orb->resolve_initial_references("SimpleDemo");

        // Narrow the reference returned.
        rootContext = CosNaming::NamingContext::_narrow(obj);

        if (CORBA::is_nil(rootContext)) {
            std::cerr << "Failed to narrow the root naming context.\n";
            return CORBA::Object::_nil();
        }
    }
    catch (CORBA::NO_RESOURCES&) {
        std::cerr << "Caught NO_RESOURCES exception. You must configure omniORB "
                  << "with the location\n"
                  << "of the naming service.\n";
        return CORBA::Object::_nil();
    }
    catch (CORBA::ORB::InvalidName &ex) {
        // This should not happen!
        std::cerr << "Service required is invalid [does not exist].\n";
        return CORBA::Object::_nil();
    }

    // Create a name object, containing the name test/context:
    CosNaming::Name name;
    name.length(2);

    name[0].id   = (const char*) "test";       // string copied
    name[0].kind = (const char*) "my_context"; // string copied
    name[1].id   = (const char*) "Echo";
    name[1].kind = (const char*) "Object";
    // Note on kind: The kind field is used to indicate the type
    // of the object. This is to avoid conventions such as that used
    // by files (name.type -- e.g. test.ps = postscript etc.)

    try {
        // Resolve the name to an object reference.
        return rootContext->resolve(name);
    }
    catch (CosNaming::NamingContext::NotFound &ex) {
        // This exception is thrown if any of the components of the
        // path [contexts or the object] aren't found:
        std::cerr << "Context not found.\n";
    }
    catch (CORBA::TRANSIENT& ex) {
        std::cerr << "Caught system exception TRANSIENT -- unable to contact the "
                  << "naming service.\n"
                  << "Make sure the naming server is running and that omniORB is "
                  << "configured correctly.\n";
    }
    catch (CORBA::SystemException& ex) {
        std::cerr << "Caught a CORBA::" << ex._name()
                  << " while using the naming service.\n";
    }
    return CORBA::Object::_nil();
}
