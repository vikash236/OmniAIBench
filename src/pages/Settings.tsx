import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { Settings as SettingsIcon, Moon, Sun, RefreshCw, Trash2, Info } from 'lucide-react';

export default function Settings() {
    return (
        <div className="space-y-4">
            {/* Header */}
            <div className="pb-3 border-b border-border">
                <h1 className="text-base font-semibold">Settings</h1>
                <p className="text-xs text-muted-foreground mt-0.5">
                    Configure application preferences
                </p>
            </div>

            {/* Appearance */}
            <Card className="border border-border bg-card">
                <CardHeader className="pb-3">
                    <CardTitle className="flex items-center gap-2 text-sm">
                        <Moon className="h-4 w-4 text-primary" />
                        Appearance
                    </CardTitle>
                </CardHeader>
                <CardContent className="space-y-3">
                    <div className="flex items-center justify-between">
                        <div>
                            <div className="text-xs font-medium">Theme</div>
                            <div className="text-xs text-muted-foreground">Current: Dark</div>
                        </div>
                        <Button variant="outline" size="sm" className="text-xs h-7">
                            <Sun className="h-3 w-3 mr-1.5" />
                            Toggle Theme
                        </Button>
                    </div>
                </CardContent>
            </Card>

            {/* Monitoring */}
            <Card className="border border-border bg-card">
                <CardHeader className="pb-3">
                    <CardTitle className="flex items-center gap-2 text-sm">
                        <RefreshCw className="h-4 w-4 text-secondary" />
                        Monitoring
                    </CardTitle>
                </CardHeader>
                <CardContent className="space-y-2.5 text-xs">
                    <div className="flex items-center justify-between">
                        <span className="text-muted-foreground">Dashboard Refresh Rate</span>
                        <span className="font-mono">5s</span>
                    </div>
                    <div className="flex items-center justify-between">
                        <span className="text-muted-foreground">Sensor Panel Refresh Rate</span>
                        <span className="font-mono">2s</span>
                    </div>
                </CardContent>
            </Card>

            {/* Benchmarks */}
            <Card className="border border-border bg-card">
                <CardHeader className="pb-3">
                    <CardTitle className="flex items-center gap-2 text-sm">
                        <SettingsIcon className="h-4 w-4 text-accent" />
                        Benchmark Settings
                    </CardTitle>
                </CardHeader>
                <CardContent className="space-y-2.5 text-xs">
                    <div className="flex items-center justify-between">
                        <span className="text-muted-foreground">CPU Test Duration</span>
                        <span className="font-mono">5 seconds</span>
                    </div>
                    <div className="flex items-center justify-between">
                        <span className="text-muted-foreground">GPU Test Duration</span>
                        <span className="font-mono">8 seconds</span>
                    </div>
                    <div className="flex items-center justify-between">
                        <span className="text-muted-foreground">NPU Test Duration</span>
                        <span className="font-mono">4 seconds</span>
                    </div>
                </CardContent>
            </Card>

            {/* Data Management */}
            <Card className="border border-border bg-card">
                <CardHeader className="pb-3">
                    <CardTitle className="flex items-center gap-2 text-sm">
                        <Trash2 className="h-4 w-4 text-danger" />
                        Data Management
                    </CardTitle>
                </CardHeader>
                <CardContent className="space-y-2">
                    <Button variant="outline" className="w-full text-xs h-8 text-danger border-danger/50 hover:bg-danger/10">
                        <Trash2 className="h-3 w-3 mr-1.5" />
                        Clear Benchmark History
                    </Button>
                    <p className="text-xs text-muted-foreground">
                        This will delete all saved benchmark results
                    </p>
                </CardContent>
            </Card>

            {/* About */}
            <Card className="border border-border bg-card">
                <CardHeader className="pb-3">
                    <CardTitle className="flex items-center gap-2 text-sm">
                        <Info className="h-4 w-4 text-info" />
                        About
                    </CardTitle>
                </CardHeader>
                <CardContent className="text-xs space-y-1.5">
                    <div className="flex justify-between">
                        <span className="text-muted-foreground">Version</span>
                        <span className="font-mono">1.0.0</span>
                    </div>
                    <div className="flex justify-between">
                        <span className="text-muted-foreground">Platform</span>
                        <span>Windows</span>
                    </div>
                    <div className="flex justify-between">
                        <span className="text-muted-foreground">Build</span>
                        <span className="font-mono">Tauri + React</span>
                    </div>
                </CardContent>
            </Card>
        </div>
    );
}
