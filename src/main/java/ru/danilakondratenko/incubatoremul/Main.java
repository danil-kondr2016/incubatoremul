package ru.danilakondratenko.incubatoremul;

import com.sun.net.httpserver.*;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.InetSocketAddress;

public class Main {
    public static VirtualIncubator virtualIncubator;
    public static void main(String[] args) {
        try {
            virtualIncubator = new VirtualIncubator();
            System.out.println("Starting server...");
            HttpServer server = HttpServer.create(new InetSocketAddress(80), 0);
            server.createContext("/", new HttpRequestHandler());
            server.start();
        } catch (Exception e) {
            e.printStackTrace();
        }

    }

    public static class HttpRequestHandler implements HttpHandler {
        public static final String DOCTYPE_HTML4 =
                "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">";
        public static final String FOOTER =
                "<p><i>IncubatorEmulator Java</i></p>";
        public static final String ROOT_PAGE =
                DOCTYPE_HTML4 +
                "<html>\n" +
                "<head><title>Incubator</title></head>\n" +
                "<body>\n" +
                "<h1 align=\"center\">The IoT-incubator</h1>\n" +
                "<p align=\"center\">Created by Kondratenko Daniel in 2021</p>\n" +
                "<hr>\n" +
                FOOTER + "\n" +
                "</body>\n" +
                "</html>\n";
        public static final String ERROR_404_PAGE = "\n" +
                DOCTYPE_HTML4 +
                "<html>\n" +
                "<head><title>Error</title></head>\n" +
                "<body>\n" +
                "<h1 align=\"center\">Error 404: page not found</h1>\n" +
                "<hr>\n" +
                FOOTER + "\n" +
                "</body>\n" +
                "</html>\n";

        @Override
        public void handle(HttpExchange exchange) throws IOException {
            String path = exchange.getRequestURI().getPath();
            String method = exchange.getRequestMethod();
            DataInputStream dis = new DataInputStream(exchange.getRequestBody());
            DataOutputStream dos = new DataOutputStream(exchange.getResponseBody());
            byte[] answerBuf = null;
            int answerLen = 0;
            if (path.compareTo("/") == 0) {
                answerBuf = ROOT_PAGE.getBytes();
                answerLen = answerBuf.length;
            } else if (path.compareTo("/control") == 0) {
                if (method.compareTo("GET") == 0) {
                    answerBuf = "method_get\r\n".getBytes();
                    answerLen = answerBuf.length;
                } else if (method.compareTo("POST") == 0) {
                    byte[] reqBuf = new byte[dis.available()];
                    dis.read(reqBuf);

                    String[] request = new String(reqBuf).replace("\r\n", "\n").split("\n");
                    System.out.println("request: " + request[0]);
                    for (String line : request) {
                        String[] args = line.trim().split(" ");
                        if (args.length == 0)
                            continue;
                        if (args[0].compareTo("request_state") == 0) {
                            answerBuf = virtualIncubator.state.serialize().getBytes();
                            answerLen = answerBuf.length;
                        } else if (args[0].compareTo("request_config") == 0) {
                            answerBuf = virtualIncubator.cfg.serialize().getBytes();
                            answerLen = answerBuf.length;
                        } else if (args[0].compareTo("lights_state") == 0) {
                            if (virtualIncubator.state.lights) {
                                answerBuf = "lights_on\r\n".getBytes();
                            } else {
                                answerBuf = "lights_off\r\n".getBytes();
                            }
                            answerLen = answerBuf.length;
                        } else {
                            if (args[0].compareTo("needed_temp") == 0) {
                                virtualIncubator.cfg.neededTemperature = Float.parseFloat(args[1]);
                            } else if (args[0].compareTo("needed_humid") == 0) {
                                virtualIncubator.cfg.neededHumidity = Float.parseFloat(args[1]);
                            } else if (args[0].compareTo("rotations_per_day") == 0) {
                                virtualIncubator.cfg.rotationsPerDay = Integer.parseInt(args[1]);
                            } else if (args[0].compareTo("switch_to_program") == 0) {
                                virtualIncubator.cfg.currentProgram = Integer.parseInt(args[1]);
                            } else if (args[0].compareTo("rotate_right") == 0) {
                                virtualIncubator.state.chamber += 1;
                                if (virtualIncubator.state.chamber > IncubatorState.CHAMBER_RIGHT)
                                    virtualIncubator.state.chamber = IncubatorState.CHAMBER_RIGHT;
                            } else if (args[0].compareTo("rotate_left") == 0) {
                                virtualIncubator.state.chamber -= 1;
                                if (virtualIncubator.state.chamber < IncubatorState.CHAMBER_LEFT)
                                    virtualIncubator.state.chamber = IncubatorState.CHAMBER_LEFT;
                            } else if (args[0].compareTo("lights_off") == 0) {
                                virtualIncubator.state.lights = false;
                            } else if (args[0].compareTo("lights_on") == 0) {
                                virtualIncubator.state.lights = true;
                            }
                            answerBuf = "success\r\n".getBytes();
                            answerLen = answerBuf.length;
                        }
                    }
                }

                exchange.getResponseHeaders().add("Content-Type", "text/plain; charset=utf-8");
                exchange.sendResponseHeaders(200, answerLen);
            } else {
                exchange.getResponseHeaders().add("Content-Type", "text/html; charset=utf-8");
                exchange.sendResponseHeaders(404, answerLen);
            }
            dos.write(answerBuf, 0, answerLen);
            dos.flush();
            dos.close();
        }
    }
}
