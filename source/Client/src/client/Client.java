package client;

import Inteface.ChatJRame;
import Inteface.LoginJFrame;
import java.io.*;
import static java.lang.Boolean.TRUE;
import java.net.*;
import java.util.TimerTask;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.swing.JOptionPane;
import javax.swing.Timer;

public class Client {   
    public static int port;
    public static String host;
    public ChatJRame chatJF ;
    
    public Handle hd;
    public Thread thr;
    public BufferedWriter os = null;
    public BufferedReader is = null;
    public BufferedWriter os1 = null;
    public BufferedReader is1 = null;
    public LoginJFrame loginJF;
    Socket socketOfClient = null;
    Socket socketOfClient1 = null;
    public byte[] FileShare;
    public String FileName;
    String filename;
    Thread t;
    public Client()
    {
        this.chatJF = new ChatJRame(this);
        this.loginJF = new LoginJFrame(this);
        loginJF.setVisible(true); 
    }
    
    public  void SendMsg(BufferedWriter os, String msg ) throws IOException
    {
        os.flush();
        os.write(msg);
       // os.newLine();
        os.flush();
        
    }
    
    public void SendByte(byte[] b) throws IOException
    {
         OutputStream out = socketOfClient.getOutputStream();
         out.write(b,0,b.length);
    }
    
    public  String RecMsg(BufferedReader is) throws IOException
    {
           String msg;
           msg = is.readLine();
           return msg;
    }
    
    public  void Login(BufferedWriter os, String username, String password, String ip, String port) throws IOException
    {
        String msg, type;
        type = "LOGIN";
        msg = type + "/" + username + "/" + password + "/" + ip + "/" +port+  "/"; 
        // gui tin nhan
        SendMsg(os, msg);
        startHandle_message();
    }
    
   public void startHandle_message()
    {
        Handle hd = new Handle();
        thr = new Thread(hd);
        thr.start();
        t = new Thread(new Runnable() {

            @Override
            public void run() {
                for(; ; ){
                    try {
                        OnlineList(os);
                    } catch (IOException ex) {
                        Logger.getLogger(Client.class.getName()).log(Level.SEVERE, null, ex);
                    }
                    try {
                        Thread.sleep(5000);
                    } catch (InterruptedException ex) {
                        Logger.getLogger(Client.class.getName()).log(Level.SEVERE, null, ex);
                    }                    
                }
            }
        });
    }
    
   public void stopHandle_message()
    {
        thr.stop();
    }
    public  void ChangeAc(BufferedWriter os,String CurrentUsername,String CurrentPass, String NewFullname, String NewUsername, String NewPassword) throws IOException
    {
        String msg, type;
        type = "CHANGE_ACCT";
        msg = type + "/"+ CurrentUsername + "/" + CurrentPass + "/" + NewFullname + "/" + NewUsername + "/"+NewPassword + "/";
        // gui tin nhan
        SendMsg(os, msg);
    }
    
    public  void OnlineList(BufferedWriter os) throws IOException
    {
        String msg, type;
        type = "ONLINE_LIST/";
        msg = type ;
        // gui tin nhan
        SendMsg(os, msg);
    }
    
    public  void Register(BufferedWriter os,String fullname, String username, String password) throws IOException
    {
        String msg, type;
        type = "REGISTER";
        msg = type + "/"  + fullname + "/" + username + "/" + password +"/"; 
        // gui tin nhan
        SendMsg(os, msg);
        startHandle_message();
    }
    
    public  void SendPrivateMsg(BufferedWriter os,String message, String target  ) throws IOException
    {
        String msg, type;
        type = "PRIVATE_MESSAGE";
        msg = type + "/" + message + "/" + target + "/";
        // gui tin nhan
        SendMsg(os, msg);
    } 
    
    public  void setupSendFile(BufferedWriter os ,String target  ) throws IOException
    {
        String msg, type;
        type = "SETUPSENDFILE";
        msg = type + "/" + FileName + "/"+ target + "/";
        // gui tin nhan
        SendMsg(os, msg);
    }
    
    public  void SendPublicMsg(BufferedWriter os, String message) throws IOException
    {
        String msg, type;
        type = "PUBLIC_MESSAGE";
        msg = type + "/"  + message + "/"; 
        // gui tin nhan
        SendMsg(os, msg);
    }   
    
    public void Logout(BufferedWriter os) throws IOException
    {
        String msg;
        msg = "LOGOUT/";
        SendMsg(os, msg);
    }  
    
    
    
    public  void handle_message_server(BufferedReader is ) throws IOException, InterruptedException
    {
        // Nhan msg   
        while(TRUE)
        {
            String msgRe = "";
            
            msgRe = RecMsg(is);
            //luc nhan ko dc
            System.out.println("Tin nhan tu Server: " + msgRe);
            String[] result = msgRe.split("/");
            String msg = result[0];
            msg = msg.trim();
            
            
            if( msg.equals("LOGIN_SUCCESS"))
            {
                loginJF.setVisible(false);
                chatJF.setVisible(true);
                chatJF.jButton4.setText(loginJF.usernameLog);
                //t = new 
                t.start();
            }
            if( msg.equals("LOGIN_FAIL"))
            {
                JOptionPane.showMessageDialog(loginJF,"Username and password incorrect.","Error",JOptionPane.WARNING_MESSAGE);
            }
            if( msg.equals("REGISTER_SUCCESS"))
            {
                JOptionPane.showMessageDialog(loginJF," Register sucessed !");
            }
            if( msg.equals("REGISTER_FAIL"))
            {
                JOptionPane.showMessageDialog(loginJF,"Existing username.","Error",JOptionPane.WARNING_MESSAGE);
            }
            if( msg.equals("CHANGE_SUCCESS"))
            {
                JOptionPane.showMessageDialog(loginJF," Change Sucessed !");
            }
            if( msg.equals("ONLINE_LIST"))
            {
                for(String r : result){
                    r = r.trim();
                    if(!r.equals("ONLINE_LIST") && !r.equals(loginJF.usernameLog))
                    {
                            int t = 0;
                            for(int i = 1; i < chatJF.model.getSize(); i++)
                                if( chatJF.model.getElementAt(i).toString().equals(r) )
                                {
                                    t++;
                                }
                            if( t != 1)
                                chatJF.model.addElement(r);
                    }
                }
            }
            
            if( msg.equals("DISCONNECT"))
            {
                for(String r : result){
                    r = r.trim();
                    if(!r.equals("DISCONNECT"))
                    {
                            int t = 0;
                            for(int i = 1; i < chatJF.model.getSize(); i++)
                                if( chatJF.model.getElementAt(i).toString().equals(r) )
                                {
                                    t++;
                                }
                            if( t == 1)
                                chatJF.model.removeElement(r);
                    }
                }
            }
            if( msg.equals("PRIVATE_MESSAGE"))
            {
                chatJF.jTextArea1.append("[ " + result[2] +" -> Me ]: " + result[1] + "\n");
            }
            if( msg.equals("PUBLIC_MESSAGE"))
            {
                chatJF.jTextArea1.append("[ " + result[2] +" -> ALL ]: " + result[1] + "\n");            
            }
                        
            if( msg.equals("SETUPSENDFILE"))
            {
                filename = result[1];
                 System.out.println("Tin nhan trong SETUP tu Server: " + msg + "..." + result[1] + "..." + result[2]);
            }
            if(msg.equals("PORT"))
            {
                //String msgR1 = "";
                //msgR1 = RecMsg(is);
               
                //System.out.println("Tin nhan trong PORT tu Server: " + msgR1);
                //String[] result1 = msgR1.split("/");
                //String msg1 = result1[0];
                //msg1 = msg1.trim();
                int portp = Integer.parseInt(result[2]);

                socketOfClient1 = new Socket(host,portp);
                os1 = new BufferedWriter(new OutputStreamWriter(socketOfClient1.getOutputStream()));
                is1 = new BufferedReader(new InputStreamReader(socketOfClient1.getInputStream()));
                
                System.out.println("result[1]: " + result[1]);
                if(result[1].equals("RECV"))
                {
                    //System.out.println("RECV haha"); // RECV/21312312312312
                    //411231
                    String msg1;
                    SendMsg(os1,"RECV/");
                    File file = new File(filename);
                    byte[] bytes = new byte[8192];
                    FileOutputStream fos = new FileOutputStream(file);
                    BufferedInputStream bis = new BufferedInputStream(socketOfClient1.getInputStream());
                    
                    int count,file_size;

                    while ((count = bis.read(bytes)) > 0) {
                        System.out.println(count);
                        fos.write(bytes, 0, count);
                    }
                    fos.flush();
                    fos.close();
                    fos.close();
                    bis.close();
                }
                else if(result[1].equals("SEND"))
                {
                    //System.out.println("SEND haha");
                    SendMsg(os1,"SEND/");
                    //os1.write(FileShare, 0, FileShare.length);
                     OutputStream out = socketOfClient1.getOutputStream();
                     out.write(FileShare, 0 ,FileShare.length);
                     out.flush();
                }
                
            }
            if( msg.equals("LOGOUT_SUCCESS"))
            {
                JOptionPane.showMessageDialog(loginJF," Logout success !");
                loginJF.setVisible(true);
                chatJF.setVisible(false); 
                stopHandle_message();
            }
            
        }

       is.close();  
    }
    public  Socket Connect() throws IOException 
    {
       try {
           // Gửi yêu cầu kết nối tới Server đang lắng nghe
           // trên máy 'localhost' cổng 9999.
           socketOfClient = new Socket(host, port);
          
          
       } catch (UnknownHostException e) {
           System.err.println("Don't know about host " + host);
           
       } catch (IOException e) {
           System.err.println("Couldn't get I/O for the connection to " + host);
           
       }
       try {
            os = new BufferedWriter(new OutputStreamWriter(socketOfClient.getOutputStream()));
       } catch (IOException ex) {
            Logger.getLogger(LoginJFrame.class.getName()).log(Level.SEVERE, null, ex);
       }

       try {
            is = new BufferedReader(new InputStreamReader(socketOfClient.getInputStream()));
        } catch (IOException ex) {
            Logger.getLogger(LoginJFrame.class.getName()).log(Level.SEVERE, null, ex);
        }
       
       return socketOfClient;
    }
    /**
     * @param args the command line arguments
     */
    public static void main(String args[]) {
        /* Set the Nimbus look and feel */
        //<editor-fold defaultstate="collapsed" desc=" Look and feel setting code (optional) ">
        /* If Nimbus (introduced in Java SE 6) is not available, stay with the default look and feel.
         * For details see http://download.oracle.com/javase/tutorial/uiswing/lookandfeel/plaf.html 
         */
        try {
            for (javax.swing.UIManager.LookAndFeelInfo info : javax.swing.UIManager.getInstalledLookAndFeels()) {
                if ("Nimbus".equals(info.getName())) {
                    javax.swing.UIManager.setLookAndFeel(info.getClassName());
                    break;
                }
            }
        } catch (ClassNotFoundException ex) {
            java.util.logging.Logger.getLogger(LoginJFrame.class.getName()).log(java.util.logging.Level.SEVERE, null, ex);
        } catch (InstantiationException ex) {
            java.util.logging.Logger.getLogger(LoginJFrame.class.getName()).log(java.util.logging.Level.SEVERE, null, ex);
        } catch (IllegalAccessException ex) {
            java.util.logging.Logger.getLogger(LoginJFrame.class.getName()).log(java.util.logging.Level.SEVERE, null, ex);
        } catch (javax.swing.UnsupportedLookAndFeelException ex) {
            java.util.logging.Logger.getLogger(LoginJFrame.class.getName()).log(java.util.logging.Level.SEVERE, null, ex);
        }
        //</editor-fold>

        /* Create and display the form */
        java.awt.EventQueue.invokeLater(new Runnable() {
            public void run() {
                new Client();
            }
        });
    } 
    public class Handle implements Runnable
    {

    @Override
    public void run() {
        
        try {
            handle_message_server(is);    
            
        } catch (IOException ex) {
            Logger.getLogger(Handle.class.getName()).log(Level.SEVERE, null, ex);
        } catch (InterruptedException ex) {
            Logger.getLogger(Handle.class.getName()).log(Level.SEVERE, null, ex);
        }
        
    }

}
}
