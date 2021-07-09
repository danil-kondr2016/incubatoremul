package ru.danilakondratenko.incubatoremul;

import java.util.concurrent.TimeUnit;

public class VirtualIncubator extends Thread {
    public IncubatorState state;
    public IncubatorConfig cfg;

    private long lastRotation = 0;
    private long startTime = 0;

    VirtualIncubator() {
        this.state = new IncubatorState();
        this.state.internet = true;
        this.state.power = true;
        this.state.currentTemperature = 36.0f;
        this.state.currentHumidity = 50.0f;
        this.state.cooler = true;
        this.state.chamber = IncubatorState.CHAMBER_NEUTRAL;

        this.cfg = new IncubatorConfig();
        this.cfg.neededTemperature = 37.5f;
        this.cfg.neededHumidity = 50.0f;
        this.cfg.rotationsPerDay = 4320;
        this.cfg.numberOfPrograms = 2;
        this.cfg.currentProgram = 0;

        startTime = System.currentTimeMillis();
        lastRotation = System.currentTimeMillis();

        System.out.println("Starting virtual incubator...");
        this.setDaemon(true);
        this.start();
    }

    private void processTemperature() {
        if (state.heater) {
            state.currentTemperature += 0.005f;
        }
        state.currentTemperature -= 0.001f;
        if (state.currentTemperature <= cfg.neededTemperature - 0.5f)
            state.heater = true;
        else if (state.currentTemperature >= cfg.neededTemperature)
            state.heater = false;
    }

    private void processHumidity() {
        state.currentHumidity -= 0.01;
        if (state.currentHumidity <= cfg.neededHumidity - 10) {
            state.currentHumidity = cfg.neededHumidity;
            state.wetter = true;
        } else if (state.currentHumidity <= cfg.neededHumidity - 0.2) {
            state.wetter = false;
        }
    }

    private void processChamber() {
        long period = 0;
        if (cfg.rotationsPerDay == 0) {
            state.chamber = IncubatorState.CHAMBER_NEUTRAL;
            return;
        }

        period = (86_400_000L / cfg.rotationsPerDay);
        if ((System.currentTimeMillis() - lastRotation) >= period) {
            switch (state.chamber) {
                case IncubatorState.CHAMBER_NEUTRAL:
                case IncubatorState.CHAMBER_LEFT:
                    state.chamber = IncubatorState.CHAMBER_RIGHT;
                    break;
                case IncubatorState.CHAMBER_RIGHT:
                    state.chamber = IncubatorState.CHAMBER_LEFT;
                    break;
            }
            lastRotation = System.currentTimeMillis();
        }
    }

    @Override
    public void run() {
        while (true) {
            this.state.timestamp = System.currentTimeMillis();
            this.state.uptime = (System.currentTimeMillis() - startTime) / 1000;
            processTemperature();
            processHumidity();
            processChamber();
            try {
                TimeUnit.MILLISECONDS.sleep(100);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }
}
