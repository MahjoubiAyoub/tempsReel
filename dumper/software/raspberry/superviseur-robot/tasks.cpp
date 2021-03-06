/*
 * Copyright (C) 2018 dimercur
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tasks.h"
#include <stdexcept>

// Déclaration des priorités des taches
#define PRIORITY_TSERVER 30
#define PRIORITY_TOPENCOMROBOT 20
#define PRIORITY_TMOVE 20
#define PRIORITY_TSENDTOMON 22
#define PRIORITY_TRECEIVEFROMMON 25
#define PRIORITY_TSTARTROBOT 20
#define PRIORITY_TCAMERA 20
#define PRIORITY_TGETBATTERY 10
#define PRIORITY_TLOSTCOMMONSUPER 21
#define PRIORITY_TLOSTCOMROBSUPER 26


/*
 * Some remarks:
 * 1- This program is mostly a template. It shows you how to create tasks, semaphore
 *   message queues, mutex ... and how to use them
 * 
 * 2- semDumber is, as name say, useless. Its goal is only to show you how to use semaphore
 * 
 * 3- Data flow is probably not optimal
 * 
 * 4- Take into account that ComRobot::Write will block your task when serial buffer is full,
 *   time for internal buffer to flush
 * 
 * 5- Same behavior existe for ComMonitor::Write !
 * 
 * 6- When you want to write something in terminal, use cout and terminate with endl and flush
 * 
 * 7- Good luck !
 */

/**
 * @brief Initialisation des structures de l'application (tâches, mutex, 
 * semaphore, etc.)
 */
void Tasks::Init() {
    int status;
    int err;

    /**************************************************************************************/
    /* 	Mutex creation                                                                    */
    /**************************************************************************************/
    if (err = rt_mutex_create(&mutex_monitor, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_robot, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_robotStarted, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_move, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    // Camera
    if (err = rt_mutex_create(&mutex_camera, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    // With WatchDog
    if (err = rt_mutex_create(&mutex_withWd, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    cout << "Mutexes created successfully" << endl << flush;

    /**************************************************************************************/
    /* 	Semaphors creation       							  */
    /**************************************************************************************/
    if (err = rt_sem_create(&sem_barrier, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_openComRobot, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_serverOk, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_startRobot, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_startRobotWD, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    // Sem ServerTAsk
    if (err = rt_sem_create(&sem_server, NULL, 1, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    // Camera
    if (err = rt_sem_create(&sem_camera, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    // lOST COMMUNICATION MONITOR SUPERVISEUR
    if (err = rt_sem_create(&sem_lostComMonSuper, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Semaphores created successfully" << endl << flush;

    /**************************************************************************************/
    /* Tasks creation                                                                     */
    /**************************************************************************************/
    if (err = rt_task_create(&th_server, "th_server", 0, PRIORITY_TSERVER, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_sendToMon, "th_sendToMon", 0, PRIORITY_TSENDTOMON, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_receiveFromMon, "th_receiveFromMon", 0, PRIORITY_TRECEIVEFROMMON, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_openComRobot, "th_openComRobot", 0, PRIORITY_TOPENCOMROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_startRobot, "th_startRobot", 0, PRIORITY_TSTARTROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_move, "th_move", 0, PRIORITY_TMOVE, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    // Thread for battery level
    if (err = rt_task_create(&th_getBatteryLevel, "th_getBatteryLevel", 0, PRIORITY_TGETBATTERY, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    // Thread for WithWatchDog
    if (err = rt_task_create(&th_startRobotWD, "th_startRobotWD", 0, PRIORITY_TSTARTROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    // Thread for the ping
    if (err = rt_task_create(&th_pingRobot, "th_pingRobot", 0, PRIORITY_TSTARTROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    // Lost communication between mon et super
    if (err = rt_task_create(&th_lostComMonSuper, "th_lostComMonSuper", 0, PRIORITY_TLOSTCOMMONSUPER, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    // Lost communication between robot et super
    if (err = rt_task_create(&th_lostComRobSuper, "th_lostComRobSuper", 0, PRIORITY_TLOSTCOMROBSUPER, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Tasks created successfully" << endl << flush;

    /**************************************************************************************/
    /* Message queues creation                                                            */
    /**************************************************************************************/
    if ((err = rt_queue_create(&q_messageToMon, "q_messageToMon", sizeof (Message*)*50, Q_UNLIMITED, Q_FIFO)) < 0) {
        cerr << "Error msg queue create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if ((err = rt_queue_create(&q_messageToRobot, "q_messageToRobot", sizeof (Message*)*100, Q_UNLIMITED, Q_FIFO)) < 0) {
        cerr << "Error msg queue create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Queues created successfully" << endl << flush;

}

/**
 * @brief Démarrage des tâches
 */
void Tasks::Run() {
    rt_task_set_priority(NULL, T_LOPRIO);
    int err;

    if (err = rt_task_start(&th_server, (void(*)(void*)) & Tasks::ServerTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_sendToMon, (void(*)(void*)) & Tasks::SendToMonTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_receiveFromMon, (void(*)(void*)) & Tasks::ReceiveFromMonTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_openComRobot, (void(*)(void*)) & Tasks::OpenComRobot, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_startRobot, (void(*)(void*)) & Tasks::StartRobotTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_move, (void(*)(void*)) & Tasks::MoveTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    // Thread for the battery level
    if (err = rt_task_start(&th_getBatteryLevel, (void(*)(void*)) & Tasks::getBatteryLevel, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    // Thread for watchwithdog
    if (err = rt_task_start(&th_startRobotWD, (void(*)(void*)) & Tasks::StartRobotTaskWD, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    // lost communocation betweeen monitor et super
    if (err = rt_task_start(&th_lostComMonSuper, (void(*)(void*)) & Tasks::LostComBetweenMonSuperTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    // lost communocation betweeen robot et super
    if (err = rt_task_start(&th_lostComRobSuper, (void(*)(void*)) & Tasks::LostComBetweenRobSuperTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }

    cout << "Tasks launched" << endl << flush;
}

/**
 * @brief Arrêt des tâches
 */
void Tasks::Stop() {
    monitor.Close();
    robot.Close();
}

/**
 */
void Tasks::Join() {
    cout << "Tasks synchronized" << endl << flush;
    rt_sem_broadcast(&sem_barrier);
    pause();
}

/**
 * @brief Thread sending data to monitor.
 */
void Tasks::SendToMonTask(void* arg) {
    Message *msg;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task sendToMon starts here                                                     */
    /**************************************************************************************/
    rt_sem_p(&sem_serverOk, TM_INFINITE);

    while (1) {
        cout << "wait msg to send" << endl << flush;
        msg = ReadInQueue(&q_messageToMon);
        cout << "Send msg to mon: " << msg->ToString() << endl << flush;
        rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
        monitor.Write(msg); // The message is deleted with the Write
        rt_mutex_release(&mutex_monitor);
    }
}

/**
 * @brief Thread receiving data from monitor.
 */
void Tasks::ReceiveFromMonTask(void *arg) {
    Message *msgRcv;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task receiveFromMon starts here                                                */
    /**************************************************************************************/
    rt_sem_p(&sem_serverOk, TM_INFINITE);
    cout << "Received message from monitor activated" << endl << flush;

    while (1) {
        msgRcv = monitor.Read();
        cout << "Rcv <= " << msgRcv->ToString() << endl << flush;

        if (msgRcv->CompareID(MESSAGE_MONITOR_LOST)) {
            rt_sem_v(&sem_lostComMonSuper);
            delete(msgRcv);
            rt_task_delete(&th_receiveFromMon);
            
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_COM_OPEN)) {
            rt_sem_v(&sem_openComRobot);
            
        }
        // Without WatchDog
        else if (msgRcv->CompareID(MESSAGE_ROBOT_START_WITHOUT_WD)) {
            rt_mutex_acquire(&mutex_withWd, TM_INFINITE);
            withWd = 1;
            rt_mutex_release(&mutex_withWd);
            rt_sem_v(&sem_startRobot);
            
        }
        // Without WatchDog
        else if (msgRcv->CompareID(MESSAGE_ROBOT_START_WITH_WD)) {            
            rt_mutex_acquire(&mutex_withWd, TM_INFINITE);
            withWd = 0;
            rt_mutex_release(&mutex_withWd);
            rt_sem_v(&sem_startRobotWD);
        }
        // Les mouvements du Robot
        else if (msgRcv->CompareID(MESSAGE_ROBOT_GO_FORWARD) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_BACKWARD) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_LEFT) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_RIGHT) ||
                msgRcv->CompareID(MESSAGE_ROBOT_STOP)) {

            rt_mutex_acquire(&mutex_move, TM_INFINITE);
            move = msgRcv->GetID();
            rt_mutex_release(&mutex_move);
        }
        // Camera ToDo
        else if (msgRcv->CompareID(MESSAGE_ROBOT_GO_FORWARD) ||
            msgRcv->CompareID(MESSAGE_ROBOT_GO_BACKWARD) ||
            msgRcv->CompareID(MESSAGE_ROBOT_GO_LEFT) ||
            msgRcv->CompareID(MESSAGE_ROBOT_GO_RIGHT) ||
            msgRcv->CompareID(MESSAGE_ROBOT_STOP)) {

            rt_mutex_acquire(&mutex_move, TM_INFINITE);
            move = msgRcv->GetID();
            rt_mutex_release(&mutex_move);
        }
        //OpenCamera and CloseCamera
        else if (msgRcv->CompareID(MESSAGE_CAM_OPEN) || 
                msgRcv->CompareID(MESSAGE_CAM_CLOSE)) {

            if (msgRcv->CompareID(MESSAGE_CAM_OPEN)) {
                rt_mutex_acquire(&mutex_camera, TM_INFINITE);
                camera = 1;
                rt_mutex_release(&mutex_camera);
                if (int err = rt_task_create(&th_camera, "th_camera", 0, PRIORITY_TCAMERA, 0)) {
                    cerr << "Error task create: " << strerror(-err) << endl << flush;
                    exit(EXIT_FAILURE);
                }
                if (int err = rt_task_start(&th_camera, (void(*)(void*)) & Tasks::CameraTask, this)) {
                    cerr << "Error task start: " << strerror(-err) << endl << flush;
                    exit(EXIT_FAILURE);
                }
            } else if (msgRcv->CompareID(MESSAGE_CAM_CLOSE)) {
                rt_mutex_acquire(&mutex_camera, TM_INFINITE);
                camera = 0;
                rt_mutex_release(&mutex_camera);
            }
            rt_sem_v(&sem_camera);
        }
        delete(msgRcv);
    }
}

/**
 * @brief Thread opening communication with the robot.
 */
void Tasks::OpenComRobot(void *arg) {
    int status;
    int err;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task openComRobot starts here                                                  */
    /**************************************************************************************/
    while (1) {
        rt_sem_p(&sem_openComRobot, TM_INFINITE);
        cout << "Open serial com (";
        rt_mutex_acquire(&mutex_robot, TM_INFINITE);
        status = robot.Open();
        rt_mutex_release(&mutex_robot);
        cout << status;
        cout << ")" << endl << flush;

        Message * msgSend;
        if (status < 0) {
            msgSend = new Message(MESSAGE_ANSWER_NACK);
        } else {
            msgSend = new Message(MESSAGE_ANSWER_ACK);
        }
        WriteInQueue(&q_messageToMon, msgSend); // msgSend will be deleted by sendToMon
    }
}

/**
 * @brief Thread starting the communication with the robot.
 */
void Tasks::StartRobotTask(void *arg) {
    int withWdL;
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task startRobot starts here                                                    */
    /**************************************************************************************/
    while (1) {
        
        Message * msgSend;
        rt_sem_p(&sem_startRobot, TM_INFINITE);
        
        rt_mutex_acquire(&mutex_withWd, TM_INFINITE);
        withWdL = withWd;
        rt_mutex_release(&mutex_withWd);
                
        if (withWdL == 0) {
            cout << "Start robot without watchdog (";
            rt_mutex_acquire(&mutex_robot, TM_INFINITE);
            msgSend = robot.Write(robot.StartWithoutWD());
            rt_mutex_release(&mutex_robot);
            cout << msgSend->GetID();
            cout << ")" << endl;
            
        } else if (withWdL == 1) {
            cout << "Start robot with watchdog (";
            rt_mutex_acquire(&mutex_robot, TM_INFINITE);
            msgSend = robot.Write(robot.StartWithWD());
            rt_mutex_release(&mutex_robot);
            cout << msgSend->GetID();
            cout << ")" << endl;
            rt_sem_v(&sem_startRobotWD);
        }
                
        WriteInQueue(&q_messageToRobot, msgSend); //to verify if we lost connectivity
        cout << "Movement answer: " << msgSend->ToString() << endl << flush;
        WriteInQueue(&q_messageToMon, msgSend);  // msgSend will be deleted by sendToMon

        if (msgSend->GetID() == MESSAGE_ANSWER_ACK) {
            rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
            robotStarted = 1;
            rt_mutex_release(&mutex_robotStarted);
        }
    }
}

/**
 * @brief Thread handling control of the robot.
 */
void Tasks::MoveTask(void *arg) {
    int rs;
    int cpMove;
    Message *msgSend;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task starts here                                                               */
    /**************************************************************************************/
    rt_task_set_periodic(NULL, TM_NOW, 100000000);

    while (1) {
        rt_task_wait_period(NULL);
        //cout << "Periodic movement update" << endl;
        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        rs = robotStarted;
        rt_mutex_release(&mutex_robotStarted);
        if (rs == 1) {
            rt_mutex_acquire(&mutex_move, TM_INFINITE);
            cpMove = move;
            rt_mutex_release(&mutex_move);
            
            cout << " move: " << cpMove << endl;
            
            rt_mutex_acquire(&mutex_robot, TM_INFINITE);
            msgSend = robot.Write(new Message((MessageID)cpMove));
            rt_mutex_release(&mutex_robot);
            
            // Pour verifier si on a perdu la connectivité 
            WriteInQueue(&q_messageToRobot, msgSend); 
        }
        cout << endl << flush;
    }
}

/**
 * Write a message in a given queue
 * @param queue Queue identifier
 * @param msg Message to be stored
 */
void Tasks::WriteInQueue(RT_QUEUE *queue, Message *msg) {
    int err;
    if ((err = rt_queue_write(queue, (const void *) &msg, sizeof ((const void *) &msg), Q_NORMAL)) < 0) {
        cerr << "Write in queue failed: " << strerror(-err) << endl << flush;
        throw std::runtime_error{"Error in write in queue"};
    }
}

/**
 * Read a message from a given queue, block if empty
 * @param queue Queue identifier
 * @return Message read
 */
Message *Tasks::ReadInQueue(RT_QUEUE *queue) {
    int err;
    Message *msg;

    if ((err = rt_queue_read(queue, &msg, sizeof ((void*) &msg), TM_INFINITE)) < 0) {
        cout << "Read in queue failed: " << strerror(-err) << endl << flush;
        throw std::runtime_error{"Error in read in queue"};
    }/** else {
        cout << "@msg :" << msg << endl << flush;
    } /**/

    return msg;
}

// Battery method
void Tasks::getBatteryLevel(void *arg) {
    int rs;
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    // Pour la periode
    rt_task_set_periodic(NULL, TM_NOW, 500000000);

    while (true) {
        rt_task_wait_period(NULL);
        cout << "Battery level update ";
        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        rs = robotStarted;
        rt_mutex_release(&mutex_robotStarted);

        if (rs == 1) {
            cout << "Get battery level update: " << endl;
            rt_mutex_acquire(&mutex_robot, TM_INFINITE);
            MessageBattery *batteryLevel = (MessageBattery*) robot.Write(new Message(MESSAGE_ROBOT_BATTERY_GET));
            rt_mutex_release(&mutex_robot);
            
            WriteInQueue(&q_messageToRobot, batteryLevel); 
            
            cout << "Battery level answer: " << batteryLevel->ToString() << endl << flush;
            // Vider les bufers et Envoyer au monitor
            WriteInQueue(&q_messageToMon, batteryLevel); 
        }
    }
}

/**
 * @brief Thread starting the communication with the robot with WatchDog.
 */
void Tasks::StartRobotTaskWD(void *arg) {
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task startRobot starts here                                                    */
    /**************************************************************************************/
    while (1) { 
        Message * msgSend;
        rt_sem_p(&sem_startRobotWD, TM_INFINITE);
        cout << "Start robot with watchdog (";
        
        rt_mutex_acquire(&mutex_robot, TM_INFINITE);
        msgSend = robot.Write(robot.StartWithWD());
        rt_mutex_release(&mutex_robot);
        cout << msgSend->GetID();
        cout << ")" << endl;

        cout << "Movement answer: " << msgSend->ToString() << endl << flush;
        WriteInQueue(&q_messageToMon, msgSend);  // msgSend will be deleted by sendToMon

        if (msgSend->GetID() == MESSAGE_ANSWER_ACK) {
            rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
            robotStarted = 1;
            rt_mutex_release(&mutex_robotStarted);
        }
        PingRobotWD(msgSend);
    }
}
void Tasks::PingRobotWD(void *msgSend) {
    cout << "Start PingRobot " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    // Pour la periode
    rt_task_set_periodic(NULL, TM_NOW, 1000000000);
    
    while (true) {
        rt_task_wait_period(NULL);

        cout << "Ping of superviseur to Robot (";
        rt_mutex_acquire(&mutex_robot, TM_INFINITE);
        msgSend = robot.Write(new Message(MESSAGE_ROBOT_RELOAD_WD));
        rt_mutex_release(&mutex_robot);
        cout << ")" << endl;
    }
}

// Verification des erreurs
Message* Tasks::SendToRobot(Message *message) {
    Message *messageReceive;

    // Le compteur des erreurs
    static int compteur = 1;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    rt_mutex_acquire(&mutex_robot, TM_INFINITE);
    messageReceive = robot.Write(message->Copy());
    
    if (messageReceive == NULL
        || messageReceive->CompareID(MESSAGE_ANSWER_ROBOT_UNKNOWN_COMMAND)
        || messageReceive->CompareID(MESSAGE_ANSWER_ROBOT_TIMEOUT) 
        || messageReceive->CompareID(MESSAGE_ANSWER_COM_ERROR)) {

        compteur++;
        if (compteur > 3) {
            compteur = 0;
            // Envoyer la notic au Monitor
            WriteInQueue(&q_messageToMon, new Message(MESSAGE_ANSWER_COM_ERROR));
                
            rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
            robotStarted = 0;
            rt_mutex_release(&mutex_robotStarted);
                
            // Arreter la watchdog 
            rt_task_set_periodic(&th_startRobotWD, TM_NOW, 0);
            } else
                compteur = 0;
        }
    rt_mutex_release(&mutex_robot);

    return messageReceive;
}

// Camera
void Tasks::CameraTask(void *args) {
    
    Message *msgSend;
    Camera *cam;
    ImageMat temp;
    Img image(temp);
    int cameraLocal;
    cam = new Camera(xs, 10);

    /**************************************************************************************/
    /* The task Vision starts here                                                    */
    /**************************************************************************************/
    while (1) {      
        // For open the camera       
        rt_sem_p(&sem_camera, TM_INFINITE);

        rt_mutex_acquire(&mutex_camera, TM_INFINITE);
        cameraLocal = camera;
        rt_mutex_release(&mutex_camera);
        if (cameraLocal == 1) {
            cam->Open();
            if (cam->IsOpen() == 0) { 
                // Problème dans l'ouverture de la camera
                cout << "Problem dans l'ouverture de la camera" << endl << flush;
                WriteInQueue(&q_messageToMon,new Message(MESSAGE_ANSWER_NACK));
            } else { 
                // la camera est ouvert
                cout << "Camera is open" << endl << flush;
                WriteInQueue(&q_messageToMon,new Message(MESSAGE_ANSWER_ACK));
                rt_task_set_periodic(NULL, TM_NOW, 500000000);
                while (1) {
                    
                    rt_mutex_acquire(&mutex_camera, TM_INFINITE);
                    cameraLocal = camera;
                    rt_mutex_release(&mutex_camera);

                    if (cameraLocal == 0) {
                        cam->Close();
                        delete cam;
                        cout << "Camera closing, unrestartable : " << endl << flush;
                        rt_task_delete(&th_camera);
                    } else {
                        
                        rt_task_wait_period(NULL);
                        cout << "Update for he image" << endl << flush;
                        image =cam->Grab();

                        // pour envoyer le message
                        cout << "Sending the image" << endl << flush;
                        MessageImg *msgImg = new MessageImg(MESSAGE_CAM_IMAGE, &image);
                        WriteInQueue(&q_messageToMon,msgImg);                        
                    }            
                }            
            }
            delete(msgSend);
            delete(cam);
        }
    }
}

// The commmunication between the server and the monitor
void Tasks::ServerTask(void *arg) {
    int status;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are started)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task server starts here                                                        */
    /**************************************************************************************/
    while (true) {
        rt_sem_p(&sem_server, TM_INFINITE);

        rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
        
        status = monitor.Open(SERVER_PORT);
        
        rt_mutex_release(&mutex_monitor);

        if (status < 0) 
            throw std::runtime_error {"Unable to start server on port " + std::to_string(SERVER_PORT)};
        else 
            cout << "Open server on port " << (SERVER_PORT) << " (" << status << ")" << endl;
        
        monitor.AcceptClient(); 
        cout << "Rock'n'Roll baby, client accepted!d!" << endl << flush;
        
        sConnect = 1;
        rt_sem_broadcast(&sem_serverOk);
    }
}

// Si on perd la communication entre le monitor et le superviseur
void Tasks::LostComBetweenMonSuperTask(void *args) {
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    
    // L'etablissement de la connexion
    rt_sem_p(&sem_serverOk, TM_INFINITE);    
    rt_sem_p(&sem_lostComMonSuper, TM_INFINITE);

    rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
    
    monitor.Close(); 
    
    rt_mutex_release(&mutex_monitor);
    
    cout << "The communication Monitor is closed " << endl;
    
    rt_mutex_acquire(&mutex_robot, TM_INFINITE);
    robot.Write(new Message(MESSAGE_ROBOT_STOP));
    rt_mutex_release(&mutex_robot);
    rt_mutex_acquire(&mutex_robot, TM_INFINITE);
    
    robot.Close();
    
    cout << "The commmunication Robot is closed " << endl;
    rt_mutex_release(&mutex_robot);
    camera = 0;
    
    rt_mutex_acquire(&mutex_camera, TM_INFINITE);
        
    rt_mutex_release(&mutex_camera);
    
    cout << "C'est fini" << endl; 
    
    usleep(1000);
    exit(1);
}

// Si on perd la communication entre le monitor et le superviseur
void Tasks::LostComBetweenRobSuperTask(void *args) {

    int rs = 0;
    Message *message;
    int compter = 0;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    while (1) {
        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        rs = robotStarted;
        rt_mutex_release(&mutex_robotStarted);
        if (rs == 1) {
            cout << "Waitint the msg" << endl << flush;
            message = ReadInQueue(&q_messageToRobot);
            cout << "The message ID : "<< message->GetID() << endl;
            if (message->GetID() == MESSAGE_ANSWER_ACK) {
                compter = 0;
            } else {
                compter++;
            }
            cout << "The compter lose rs : " << compter << endl;
            if (compter == 5) {

                // the sending don't figure out
                rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
                robotStarted = 0;
                rs = robotStarted;
                rt_mutex_release(&mutex_robotStarted);
                rt_mutex_acquire(&mutex_robot, TM_INFINITE);
                
                robot.Close();
                
                cout << "Communication robot close " << endl;
                
                rt_mutex_release(&mutex_robot);
                rt_sem_v(&sem_openComRobot);
            }
        }
    }
}
