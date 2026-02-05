import { useEffect, useState } from "react";
import { invoke } from "@tauri-apps/api/core";
import { Activity } from "lucide-react";
import { AuthButton } from "./AuthButton";
import { SensorPanel } from "./SensorPanel";
import { Button } from "./ui/button";

interface HardwareInfo {
    cpu?: {
        name: string;
        cores: number;
        threads: number;
        max_freq_mhz: number;
    };
    gpu?: {
        name?: string;
        vram_gb?: number;
    };
    ram?: {
        total_gb: number;
    };
    npu?: {
        name?: string;
    };
}

export function Header() {
    const [hardware, setHardware] = useState<HardwareInfo | null>(null);
    const [sensorPanelOpen, setSensorPanelOpen] = useState(false);

    useEffect(() => {
        loadHardware();

        // Auto-refresh every 2 seconds (reduced to save CPU)
        const interval = setInterval(() => {
            loadHardware();
        }, 2000);

        return () => clearInterval(interval);
    }, []);

    async function loadHardware() {
        try {
            const result = await invoke<string>("scan_hardware");
            setHardware(JSON.parse(result));
        } catch (error) {
            console.error("Failed to load hardware:", error);
        }
    }

    return (
        <>
            <header className="fixed top-0 right-0 left-64 z-40 h-16 glass border-b border-border/50">
                <div className="flex h-full items-center justify-between px-6">
                    <div className="flex items-center gap-6 text-sm">
                        <div className="flex items-center gap-2">
                            <span className="text-muted-foreground">CPU:</span>
                            <span className="font-medium text-primary">
                                {hardware?.cpu?.name || "Detecting..."}
                            </span>
                        </div>
                        {hardware?.gpu?.name && (
                            <>
                                <div className="h-4 w-px bg-border" />
                                <div className="flex items-center gap-2">
                                    <span className="text-muted-foreground">GPU:</span>
                                    <span className="font-medium text-secondary">
                                        {hardware.gpu.name}
                                    </span>
                                </div>
                            </>
                        )}
                    </div>

                    <div className="flex items-center gap-4">
                        {/* Sensor Panel Toggle */}
                        <Button
                            variant="outline"
                            size="sm"
                            onClick={() => setSensorPanelOpen(!sensorPanelOpen)}
                            className="gap-2"
                        >
                            <Activity className="h-4 w-4" />
                            <span>Sensors</span>
                        </Button>

                        <AuthButton />
                        <div className="text-sm text-muted-foreground">
                            {hardware?.ram?.total_gb ? `${hardware.ram.total_gb.toFixed(0)} GB RAM` : ""}
                        </div>
                    </div>
                </div>
            </header>

            {/* Sensor Panel Slide-out */}
            <SensorPanel isOpen={sensorPanelOpen} onClose={() => setSensorPanelOpen(false)} />
        </>
    );
}
