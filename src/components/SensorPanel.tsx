import { useState, useEffect } from 'react';
import { X, Thermometer, Gauge, Zap, Activity, Fan, Cpu, Gpu, Circle } from 'lucide-react';
import { invoke } from '@tauri-apps/api/core';
import { Button } from './ui/button';

interface SensorData {
    cpu: {
        name: string;
        cores: Array<{
            id: number;
            physical_core: boolean;
            frequency: number;
            load: number;
            temp: number | null;
        }>;
        package: {
            temp: number | null;
            power: number | null;
            voltage: number | null;
        };
    };
    gpus: Array<{
        type: string;
        vendor: string;
        name: string;
        temp: number | null;
        utilization?: { gpu: number; memory?: number };
        memory?: { total: number; used: number; free: number };
        clocks?: { core: number; memory: number };
        power?: { draw: number; limit: number };
        fan_speed?: number | null;
    }>;
    memory: {
        total_gb: number;
        used_gb: number;
        free_gb: number;
        percent_used: number;
    };
    storage: Array<{
        name: string;
        mountpoint: string;
        total_gb: number;
        used_gb: number;
        percent_used: number;
    }>;
    network?: {
        bytes_sent: number;
        bytes_recv: number;
    };
    battery?: {
        percent: number;
        plugged_in: boolean;
    } | null;
}

export function SensorPanel({ isOpen, onClose }: { isOpen: boolean; onClose: () => void }) {
    const [sensors, setSensors] = useState<SensorData | null>(null);

    useEffect(() => {
        if (!isOpen) return;

        const loadSensors = async () => {
            try {
                const result = await invoke<string>('get_detailed_sensors');
                const parsed = JSON.parse(result);
                setSensors(parsed);
            } catch (error) {
                console.error('Failed to load sensors:', error);
            }
        };

        loadSensors();
        const interval = setInterval(loadSensors, 2000);
        return () => clearInterval(interval);
    }, [isOpen]);

    const getTempColor = (temp: number | null) => {
        if (!temp) return 'text-muted-foreground';
        if (temp < 50) return 'text-success';
        if (temp < 70) return 'text-warning';
        return 'text-danger';
    };

    const getLoadColor = (load: number) => {
        if (load < 30) return 'text-success';
        if (load < 70) return 'text-warning';
        return 'text-danger';
    };

    if (!isOpen) return null;

    return (
        <div className="fixed right-0 top-0 h-screen w-96 bg-card border-l border-border z-50 overflow-y-auto">
            {/* Header */}
            <div className="sticky top-0 bg-card border-b border-border px-3 py-2 flex items-center justify-between">
                <h2 className="text-sm font-semibold">Hardware Sensors</h2>
                <Button variant="ghost" size="icon" onClick={onClose} className="h-7 w-7">
                    <X className="h-3.5 w-3.5" />
                </Button>
            </div>

            {/* Content */}
            <div className="p-3 space-y-3">
                {!sensors ? (
                    <div className="text-center py-8">
                        <div className="inline-block h-6 w-6 animate-spin rounded-full border-2 border-primary border-r-transparent" />
                        <p className="mt-2 text-xs text-muted-foreground">Loading sensors...</p>
                    </div>
                ) : (
                    <>
                        {/* Temperatures Section */}
                        <div className="border border-border rounded-md">
                            <div className="px-2.5 py-1.5 bg-card border-b border-border">
                                <div className="flex items-center gap-1.5 text-xs font-medium">
                                    <Thermometer className="h-3.5 w-3.5 text-danger" />
                                    <span>Temperatures</span>
                                </div>
                            </div>
                            <div className="p-2 space-y-1 text-xs">
                                {/* CPU Package Temp */}
                                {sensors.cpu.package.temp && (
                                    <div className="flex justify-between items-center">
                                        <div className="flex items-center gap-1">
                                            <Cpu className="h-3 w-3 text-primary" />
                                            <span className="text-muted-foreground">CPU Package</span>
                                        </div>
                                        <span className={getTempColor(sensors.cpu.package.temp)}>
                                            {sensors.cpu.package.temp.toFixed(1)}°C
                                        </span>
                                    </div>
                                )}

                                {/* CPU Core Temps */}
                                {sensors.cpu.cores
                                    .filter(core => core.physical_core && core.temp)
                                    .map(core => (
                                        <div key={core.id} className="flex justify-between items-center">
                                            <div className="flex items-center gap-1">
                                                <Circle className="h-2 w-2 text-primary" />
                                                <span className="text-muted-foreground">Core {core.id}</span>
                                            </div>
                                            <span className={getTempColor(core.temp)}>
                                                {core.temp?.toFixed(1)}°C
                                            </span>
                                        </div>
                                    ))}

                                {/* GPU Temps */}
                                {sensors.gpus.map((gpu, idx) => gpu.temp && (
                                    <div key={idx} className="flex justify-between items-center">
                                        <div className="flex items-center gap-1">
                                            <Gpu className="h-3 w-3 text-accent" />
                                            <span className="text-muted-foreground">
                                                {gpu.type === 'discrete' ? 'dGPU' : 'iGPU'}
                                            </span>
                                        </div>
                                        <span className={getTempColor(gpu.temp)}>
                                            {gpu.temp.toFixed(1)}°C
                                        </span>
                                    </div>
                                ))}
                            </div>
                        </div>

                        {/* Loads / Utilization Section */}
                        <div className="border border-border rounded-md">
                            <div className="px-2.5 py-1.5 bg-card border-b border-border">
                                <div className="flex items-center gap-1.5 text-xs font-medium">
                                    <Gauge className="h-3.5 w-3.5 text-warning" />
                                    <span>Utilization</span>
                                </div>
                            </div>
                            <div className="p-2 space-y-1 text-xs">
                                {/* CPU Load per core */}
                                {sensors.cpu.cores
                                    .filter(core => core.physical_core)
                                    .map(core => (
                                        <div key={core.id} className="flex justify-between items-center">
                                            <div className="flex items-center gap-1">
                                                <Cpu className="h-3 w-3 text-primary" />
                                                <span className="text-muted-foreground">Core {core.id}</span>
                                            </div>
                                            <span className={getLoadColor(core.load)}>
                                                {core.load.toFixed(0)}%
                                            </span>
                                        </div>
                                    ))}

                                {/* GPU Utilization */}
                                {sensors.gpus.map((gpu, idx) => gpu.utilization && (
                                    <div key={idx} className="space-y-0.5">
                                        <div className="flex justify-between items-center">
                                            <div className="flex items-center gap-1">
                                                <Gpu className="h-3 w-3 text-accent" />
                                                <span className="text-muted-foreground">GPU</span>
                                            </div>
                                            <span className={getLoadColor(gpu.utilization.gpu)}>
                                                {gpu.utilization.gpu}%
                                            </span>
                                        </div>
                                        {gpu.utilization.memory !== undefined && (
                                            <div className="flex justify-between items-center pl-4">
                                                <span className="text-muted-foreground">VRAM</span>
                                                <span className={getLoadColor(gpu.utilization.memory)}>
                                                    {gpu.utilization.memory}%
                                                </span>
                                            </div>
                                        )}
                                    </div>
                                ))}

                                {/* RAM Usage */}
                                <div className="flex justify-between items-center">
                                    <span className="text-muted-foreground">System RAM</span>
                                    <span className={getLoadColor(sensors.memory.percent_used)}>
                                        {sensors.memory.percent_used.toFixed(0)}%
                                    </span>
                                </div>
                            </div>
                        </div>

                        {/* Frequencies Section */}
                        <div className="border border-border rounded-md">
                            <div className="px-2.5 py-1.5 bg-card border-b border-border">
                                <div className="flex items-center gap-1.5 text-xs font-medium">
                                    <Activity className="h-3.5 w-3.5 text-info" />
                                    <span>Frequencies</span>
                                </div>
                            </div>
                            <div className="p-2 space-y-1 text-xs">
                                {/* CPU Core Frequencies */}
                                {sensors.cpu.cores
                                    .filter(core => core.physical_core && core.frequency > 0)
                                    .map(core => (
                                        <div key={core.id} className="flex justify-between items-center">
                                            <div className="flex items-center gap-1">
                                                <Cpu className="h-3 w-3 text-primary" />
                                                <span className="text-muted-foreground">Core {core.id}</span>
                                            </div>
                                            <span className="font-mono text-info">
                                                {(core.frequency / 1000).toFixed(2)} GHz
                                            </span>
                                        </div>
                                    ))}

                                {/* GPU Clocks */}
                                {sensors.gpus.map((gpu, idx) => gpu.clocks && (
                                    <div key={idx} className="space-y-0.5">
                                        <div className="flex justify-between items-center">
                                            <div className="flex items-center gap-1">
                                                <Gpu className="h-3 w-3 text-accent" />
                                                <span className="text-muted-foreground">GPU Core</span>
                                            </div>
                                            <span className="font-mono text-info">
                                                {gpu.clocks.core} MHz
                                            </span>
                                        </div>
                                        <div className="flex justify-between items-center pl-4">
                                            <span className="text-muted-foreground">Memory</span>
                                            <span className="font-mono text-info">
                                                {gpu.clocks.memory} MHz
                                            </span>
                                        </div>
                                    </div>
                                ))}
                            </div>
                        </div>

                        {/* Power Section */}
                        <div className="border border-border rounded-md">
                            <div className="px-2.5 py-1.5 bg-card border-b border-border">
                                <div className="flex items-center gap-1.5 text-xs font-medium">
                                    <Zap className="h-3.5 w-3.5 text-warning" />
                                    <span>Power & Voltage</span>
                                </div>
                            </div>
                            <div className="p-2 space-y-1 text-xs">
                                {/* CPU Package Power */}
                                {sensors.cpu.package.power && (
                                    <div className="flex justify-between items-center">
                                        <div className="flex items-center gap-1">
                                            <Cpu className="h-3 w-3 text-primary" />
                                            <span className="text-muted-foreground">CPU Package</span>
                                        </div>
                                        <span className="font-mono text-warning">
                                            {sensors.cpu.package.power.toFixed(1)} W
                                        </span>
                                    </div>
                                )}

                                {/* CPU Voltage */}
                                {sensors.cpu.package.voltage && (
                                    <div className="flex justify-between items-center">
                                        <div className="flex items-center gap-1">
                                            <Cpu className="h-3 w-3 text-primary" />
                                            <span className="text-muted-foreground">CPU Voltage</span>
                                        </div>
                                        <span className="font-mono text-warning">
                                            {sensors.cpu.package.voltage.toFixed(3)} V
                                        </span>
                                    </div>
                                )}

                                {/* GPU Power */}
                                {sensors.gpus.map((gpu, idx) => gpu.power && (
                                    <div key={idx} className="flex justify-between items-center">
                                        <div className="flex items-center gap-1">
                                            <Gpu className="h-3 w-3 text-accent" />
                                            <span className="text-muted-foreground">GPU Power</span>
                                        </div>
                                        <span className="font-mono text-warning">
                                            {gpu.power.draw.toFixed(1)} / {gpu.power.limit.toFixed(0)} W
                                        </span>
                                    </div>
                                ))}
                            </div>
                        </div>

                        {/* Fans Section */}
                        {sensors.gpus.some(gpu => gpu.fan_speed !== null && gpu.fan_speed !== undefined) && (
                            <div className="border border-border rounded-md">
                                <div className="px-2.5 py-1.5 bg-card border-b border-border">
                                    <div className="flex items-center gap-1.5 text-xs font-medium">
                                        <Fan className="h-3.5 w-3.5 text-info" />
                                        <span>Fans</span>
                                    </div>
                                </div>
                                <div className="p-2 space-y-1 text-xs">
                                    {sensors.gpus.map((gpu, idx) => gpu.fan_speed !== null && gpu.fan_speed !== undefined && (
                                        <div key={idx} className="flex justify-between items-center">
                                            <div className="flex items-center gap-1">
                                                <Gpu className="h-3 w-3 text-accent" />
                                                <span className="text-muted-foreground">GPU Fan</span>
                                            </div>
                                            <span className="font-mono text-info">
                                                {gpu.fan_speed}%
                                            </span>
                                        </div>
                                    ))}
                                </div>
                            </div>
                        )}

                        {/* Memory Details */}
                        <div className="border border-border rounded-md">
                            <div className="px-2.5 py-1.5 bg-card border-b border-border">
                                <div className="flex items-center gap-1.5 text-xs font-medium">
                                    <Activity className="h-3.5 w-3.5 text-secondary" />
                                    <span>Memory</span>
                                </div>
                            </div>
                            <div className="p-2 space-y-1 text-xs">
                                <div className="flex justify-between items-center">
                                    <span className="text-muted-foreground">System RAM</span>
                                    <span className="font-mono">
                                        {sensors.memory.used_gb.toFixed(1)} / {sensors.memory.total_gb.toFixed(1)} GB
                                    </span>
                                </div>
                                {sensors.gpus.map((gpu, idx) => gpu.memory && (
                                    <div key={idx} className="flex justify-between items-center">
                                        <span className="text-muted-foreground">VRAM ({gpu.type === 'discrete' ? 'dGPU' : 'iGPU'})</span>
                                        <span className="font-mono">
                                            {gpu.memory.used.toFixed(1)} / {gpu.memory.total.toFixed(1)} GB
                                        </span>
                                    </div>
                                ))}
                            </div>
                        </div>
                    </>
                )}
            </div>
        </div>
    );
}
