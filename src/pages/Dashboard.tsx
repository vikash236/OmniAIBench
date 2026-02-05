import { useState, useEffect } from "react";
import { invoke } from "@tauri-apps/api/core";
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { Cpu, MemoryStick, Monitor, Zap } from 'lucide-react';

interface SystemStats {
    cpu: {
        name: string;
        cores: number;
        threads: number;
        load: number;
        temp?: number;
    };
    ram: {
        total_gb: number;
        used_gb: number;
    };
    gpu: {
        name: string;
        vram_gb: number;
        temp?: number;
    };
    npu: {
        name: string;
        tops: number;
    } | null;
}

export default function Dashboard() {
    const [stats, setStats] = useState<SystemStats | null>(null);
    const [error, setError] = useState<string>("");

    useEffect(() => {
        loadSystemStats();
        const interval = setInterval(loadSystemStats, 5000);
        return () => clearInterval(interval);
    }, []);

    async function loadSystemStats() {
        try {
            const result = await invoke<string>("scan_hardware");
            setStats(JSON.parse(result));
            setError("");
        } catch (error) {
            console.error("Failed to load stats:", error);
            setError(String(error));
        }
    }

    if (error) {
        return (
            <div className="flex h-full items-center justify-center p-4">
                <div className="text-center max-w-2xl">
                    <div className="text-red-400 text-sm mb-2">⚠️ Hardware Scan Error</div>
                    <div className="text-xs text-muted-foreground bg-card p-3 rounded border border-border text-left">
                        <pre className="whitespace-pre-wrap font-mono">{error}</pre>
                    </div>
                    <button
                        onClick={loadSystemStats}
                        className="mt-3 px-3 py-1.5 text-xs bg-primary text-primary-foreground rounded hover:bg-primary/90"
                    >
                        Retry
                    </button>
                </div>
            </div>
        );
    }

    if (!stats) {
        return (
            <div className="flex h-full items-center justify-center">
                <div className="text-center">
                    <div className="inline-block h-8 w-8 animate-spin rounded-full border-2 border-primary border-r-transparent mb-3" />
                    <p className="text-xs text-muted-foreground">Scanning hardware...</p>
                </div>
            </div>
        );
    }

    return (
        <div className="space-y-4">
            {/* Header */}
            <div className="pb-3 border-b border-border">
                <h1 className="text-base font-semibold">System Overview</h1>
                <p className="text-xs text-muted-foreground mt-0.5">Real-time hardware monitoring</p>
            </div>

            {/* Stats Grid - Clean and minimal */}
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-3">
                {/* CPU Card */}
                <Card className="border border-border bg-card">
                    <CardHeader className="pb-2.5">
                        <CardTitle className="flex items-center gap-1.5 text-xs font-medium">
                            <Cpu className="h-3.5 w-3.5 text-primary" />
                            CPU
                        </CardTitle>
                        <CardDescription className="text-xs truncate">{stats.cpu.name}</CardDescription>
                    </CardHeader>
                    <CardContent className="space-y-1.5 text-xs">
                        <div className="flex justify-between">
                            <span className="text-muted-foreground">Cores</span>
                            <span className="font-mono">{stats.cpu.cores}</span>
                        </div>
                        <div className="flex justify-between">
                            <span className="text-muted-foreground">Threads</span>
                            <span className="font-mono">{stats.cpu.threads}</span>
                        </div>
                        <div className="flex justify-between">
                            <span className="text-muted-foreground">Load</span>
                            <span className="font-mono">{stats.cpu.load.toFixed(1)}%</span>
                        </div>
                        {stats.cpu.temp && (
                            <div className="flex justify-between">
                                <span className="text-muted-foreground">Temp</span>
                                <span className="font-mono">{stats.cpu.temp.toFixed(1)}°C</span>
                            </div>
                        )}
                    </CardContent>
                </Card>

                {/* RAM Card */}
                <Card className="border border-border bg-card">
                    <CardHeader className="pb-2.5">
                        <CardTitle className="flex items-center gap-1.5 text-xs font-medium">
                            <MemoryStick className="h-3.5 w-3.5 text-secondary" />
                            Memory
                        </CardTitle>
                    </CardHeader>
                    <CardContent className="space-y-1.5 text-xs">
                        <div className="flex justify-between">
                            <span className="text-muted-foreground">Used</span>
                            <span className="font-mono">{stats.ram.used_gb.toFixed(1)} GB</span>
                        </div>
                        <div className="flex justify-between">
                            <span className="text-muted-foreground">Total</span>
                            <span className="font-mono">{stats.ram.total_gb.toFixed(1)} GB</span>
                        </div>
                        <div className="flex justify-between">
                            <span className="text-muted-foreground">Usage</span>
                            <span className="font-mono">
                                {((stats.ram.used_gb / stats.ram.total_gb) * 100).toFixed(0)}%
                            </span>
                        </div>
                    </CardContent>
                </Card>

                {/* GPU Card */}
                <Card className="border border-border bg-card">
                    <CardHeader className="pb-2.5">
                        <CardTitle className="flex items-center gap-1.5 text-xs font-medium">
                            <Monitor className="h-3.5 w-3.5 text-accent" />
                            GPU
                        </CardTitle>
                        <CardDescription className="text-xs truncate">{stats.gpu.name}</CardDescription>
                    </CardHeader>
                    <CardContent className="space-y-1.5 text-xs">
                        <div className="flex justify-between">
                            <span className="text-muted-foreground">VRAM</span>
                            <span className="font-mono">{stats.gpu.vram_gb.toFixed(1)} GB</span>
                        </div>
                        {stats.gpu.temp && (
                            <div className="flex justify-between">
                                <span className="text-muted-foreground">Temp</span>
                                <span className="font-mono">{stats.gpu.temp.toFixed(1)}°C</span>
                            </div>
                        )}
                    </CardContent>
                </Card>

                {/* NPU Card */}
                {stats.npu && (
                    <Card className="border border-border bg-card">
                        <CardHeader className="pb-2.5">
                            <CardTitle className="flex items-center gap-1.5 text-xs font-medium">
                                <Zap className="h-3.5 w-3.5 text-warning" />
                                NPU
                            </CardTitle>
                            <CardDescription className="text-xs truncate">{stats.npu.name}</CardDescription>
                        </CardHeader>
                        <CardContent className="space-y-1.5 text-xs">
                            <div className="flex justify-between">
                                <span className="text-muted-foreground">Performance</span>
                                <span className="font-mono">{stats.npu.tops} TOPS</span>
                            </div>
                        </CardContent>
                    </Card>
                )}
            </div>
        </div>
    );
}
