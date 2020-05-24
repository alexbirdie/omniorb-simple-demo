//  This is the source code of a slightly modified example 3 used in Chapter 2
//  "The Basics" of the omniORB user guide.
//
//  This is the object implementation.
//
//  You need omniNames service running and omniORB.cfg configured
//  (try "InitRef = SimpleDemo=corbaname::localhost" for local tests)
//  as well as omniORB libraries installed in your system.
//
//  Usage:
//
//        On startup, the object reference is registered with the
//        COS naming service. The client uses the naming service to
//        locate this object.
//
//        The name which the object is bound to is as follows:
//              root  [context]
//               |
//              test  [context] kind [my_context]
//               |
//              Echo  [object]  kind [Object]
//

#include <echo.hh>

#include <iostream>

static CORBA::Boolean bindObjectToName(CORBA::ORB_ptr, CORBA::Object_ptr);


class Echo_i : public POA_Echo
{
public:
    inline Echo_i() {}
    virtual ~Echo_i() {}
    virtual char* echoString(const char* mesg);
};


char* Echo_i::echoString(const char* mesg)
{
    std::cout << mesg;
    std::cout.flush();
    return CORBA::string_dup(mesg);
}

//////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
    try {
        CORBA::ORB_var          orb = CORBA::ORB_init(argc, argv);
        CORBA::Object_var       obj = orb->resolve_initial_references("RootPOA");
        PortableServer::POA_var poa = PortableServer::POA::_narrow(obj);

        PortableServer::Servant_var<Echo_i> myEcho = new Echo_i();

        PortableServer::ObjectId_var myechoid = poa->activate_object(myEcho);

        // Obtain a reference to the object, and register it in
        // the naming service.
        obj = myEcho->_this();

        CORBA::String_var sior(orb->object_to_string(obj));
        //    std::cout << sior << '\n';

        if (!bindObjectToName(orb, obj))
            return 1;

        PortableServer::POAManager_var pMan = poa->the_POAManager();
        pMan->activate();

        std::cout << "Server is running. Awaiting connections...\n";
        orb->run();
    }
    catch (CORBA::SystemException& ex) {
        std::cerr << "Caught CORBA::" << ex._name() << '\n';
    }
    catch (CORBA::Exception& ex) {
        std::cerr << "Caught CORBA::Exception: " << ex._name() << '\n';
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////

static CORBA::Boolean bindObjectToName(CORBA::ORB_ptr orb, CORBA::Object_ptr objRef)
{
    CosNaming::NamingContext_var rootContext;

    try {
        // Obtain a reference to the root context of the Name service:
        CORBA::Object_var obj = orb->resolve_initial_references("SimpleDemo");

        // Narrow the reference returned.
        rootContext = CosNaming::NamingContext::_narrow(obj);
        if (CORBA::is_nil(rootContext)) {
            std::cerr << "Failed to narrow the root naming context." << '\n';
            return 0;
        }
    }
    catch (CORBA::NO_RESOURCES&) {
        std::cerr << "Caught NO_RESOURCES exception. You must configure omniORB\n"
                  << "with the location of the naming service.\n";
        return 0;
    }
    catch (CORBA::ORB::InvalidName&) {
        // This should not happen!
        std::cerr << "Service required is invalid [does not exist].\n";
        return 0;
    }

    try {
        // Bind a context called "test" to the root context:

        CosNaming::Name contextName;
        contextName.length(1);
        contextName[0].id   = (const char*) "test";       // string copied
        contextName[0].kind = (const char*) "my_context"; // string copied
        // Note on kind: The kind field is used to indicate the type
        // of the object. This is to avoid conventions such as that used
        // by files (name.type -- e.g. test.ps = postscript etc.)

        CosNaming::NamingContext_var testContext;
        try {
            // Bind the context to root.
            testContext = rootContext->bind_new_context(contextName);
        }
        catch(CosNaming::NamingContext::AlreadyBound &ex) {
            // If the context already exists, this exception will be raised.
            // In this case, just resolve the name and assign testContext
            // to the object returned:
            CORBA::Object_var obj = rootContext->resolve(contextName);
            testContext = CosNaming::NamingContext::_narrow(obj);
            if (CORBA::is_nil(testContext)) {
                std::cerr << "Failed to narrow naming context.\n";
                return 0;
            }
        }

        // Bind objRef with name Echo to the testContext:
        CosNaming::Name objectName;
        objectName.length(1);
        objectName[0].id   = (const char*) "Echo";   // string copied
        objectName[0].kind = (const char*) "Object"; // string copied

        try {
            testContext->bind(objectName, objRef);
        }
        catch(CosNaming::NamingContext::AlreadyBound &ex) {
            testContext->rebind(objectName, objRef);
        }
        // Note: Using rebind() will overwrite any Object previously bound
        //       to /test/Echo with obj.
        //       Alternatively, bind() can be used, which will raise a
        //       CosNaming::NamingContext::AlreadyBound exception if the name
        //       supplied is already bound to an object.
    }
    catch (CORBA::TRANSIENT &ex) {
        std::cerr << "Caught system exception TRANSIENT -- unable to contact the "
                  << "naming service.\n"
                  << "Make sure the naming server is running and that omniORB is "
                  << "configured correctly.\n";

        return 0;
    }
    catch (CORBA::SystemException &ex) {
        std::cerr << "Caught a CORBA::" << ex._name()
                  << " while using the naming service.\n";
        return 0;
    }
    return 1;
}
