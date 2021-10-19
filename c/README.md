# Machine Learning C API


## Target Platforms


### Inference.Single, Inference.Pipeline

- Tizen: Tizen public APIs of the machine learning domain.
    - Because this is public (official) APIs of the platform, there are a few restrictions incurred by user permissions and access control.
- Android: APIs offered for JNI binaries. Used in Samsung-Android products.
- Ubuntu: APIs offered
- You may build and use for arbitrary operating systems including MacOS, Yocto, and so on.

### Training

- Tizen: Tizen public APIs of the machine learning domain.
    - Because this is public (official) APIs of the platform, there are a few restrictions incurred by user permissions and access control.
- Android:
- Ubuntu:


## Design


Target Design (there are a little work-to-do to follow this design)
```(impl)``` and ```'''``` or ```:``` = Implements
``` < V ``` and ```---``` or ```|``` = Depends
``` [ ] ``` = interface (header)
``` ( ) ``` = implementation (source)

### Overview with other repos
```
+----- API.git -----------------------------------------------------------+
|                                                                         |
| [........ ml-api-common ...........]<''''(impl)'''( ml-api-common-impl) |
|    ^                           ^                                        |
|    |                           |                                        |
| [ ml-api-training ]   [ ml-api-inference ] <---- [ ml-api-service ]     |
|    ^ (impl)                    ^ (impl)                   ^ (impl)      |
|    :                           :                          :             |
| ( ml-api-tr-impl )    ( ml-api-inf-impl (p/s) )  ( ml-api-serv-impl )   |
+----|---------------------------|----------------------------------------+
     V                           V                          
+----+- nntrainer.git --+ +------+-------- nnstreamer.git ----------------+
|                       | |                       + GStreamer             |
+-----------------------+ +-----------------------------------------------+
```
- As of 2021-10-20, ml-api-tr-impl is not yet migrated from nntrainer.git.
- As of 2021-10-20, ml-api-service and ml-api-serv-impl do not exist, yet.

### Internal overview (trainer not included)
```

  [ ml-api-common (pub) ]<---+-----------------------------------+
    ^                        |                                   |
    :           [ nnstreamer.h == ml-api-inf-pipeline (pub) ]<-- | -------------------+
    :                        ^                                   |                    |
    :                        : [ nnstreamer-single.h == ml-api-inf-single (pub) ]<----+
    :                        :    ^                                                   |
    :                        :    : [ nnstreamer-tizen-internal.h == ml-api-private (platform only) ]
    :                        :    :             ^
  ( ml-api-common )          :    +'''''''''''''+
    :                        +''' : ''''''''''''+
    : ( ml-api-common-tiz* ) :    :
    :             :          :    :
    :             :          :    :
    V             V          :    :
  [ ml-api-internal (int) ]  :    +''''''''''''+
    ^                        :                 :
    |           ( ml-api-inference-pipeline )  :
    |               |                          :
    |               |          ( ml-api-inference-single )
    |               V                          |
[ ml-api-inference-internal (int) ]<-----------+
    ^
    :
( ml-api-inference-internal )

```
- (pub): public header / interface, exposed to SDK and applications
- (platform only): private header / interface, exposed to platform components
- (int): internal header / interface, exposed to api.git components only.

- ml-trainer APIs may depend on ml-api-internal.

### Headers

/c/include/*.h : Exported headers, accessed by other packages.
   - Public (ml-api-common.h, nnstreamer-single.h, nnstreamer.h)
       - Supposed to be accessed by application via SDK (e.g., Tizen Studio, Android Studio)
   - Platform Only (nnstreamer-tizen-internal.h)
       - Supposed to be accessed by platform package (e.g., Tizen middleware)
/c/src/*.h : Not exported headers. Cannot be accessed by other packages.
