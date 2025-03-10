#include "TransportComponent.h"

TransportComponent::TransportComponent(tracktion::engine::Edit& e)
    : edit(e),
      transport(e.getTransport()),
      transportBar(e),
      thumbnail(e.engine, tracktion::AudioFile(e.engine), *this, &e),
      zoomLevel(1.0),
      scrollPosition(0.0)
{
    // Add transport bar
    addAndMakeVisible(transportBar);

    // Add and make visible other components
    addAndMakeVisible(pluginAutomationViewport);
    pluginAutomationViewport.setViewedComponent(&pluginAutomationContainer, false);
    pluginAutomationViewport.setScrollBarsShown(true, false);

    // Initialize thumbnail
    thumbnail.audioFileChanged();
    startTimerHz(30);

    // Setup playhead
    playhead = std::make_unique<juce::DrawableRectangle>();
    playhead->setFill(juce::FillType(juce::Colours::red));
    addAndMakeVisible(*playhead);

    // Create and add crossfader automation lane
    crossfaderAutomationLane = std::make_unique<CrossfaderAutomationLane>(edit);
    
    // Find the crossfader parameter
    tracktion::engine::AutomatableParameter* crossfaderParam = nullptr;
    if (auto chopPlugin = EngineHelpers::getPluginFromMasterTrack(edit, ChopPlugin::xmlTypeName))
    {
        if (auto* plugin = chopPlugin.get())
        {
            crossfaderParam = plugin->getAutomatableParameterByID("crossfader");
        }
    }
    
    if (crossfaderParam != nullptr)
    {
        crossfaderAutomationLane->setParameter(crossfaderParam);
    }
    
    addAndMakeVisible(*crossfaderAutomationLane);

    // Create and add automation lane for reverb wet parameter
    reverbWetAutomationLane = std::make_unique<AutomationLane>(edit);
    
    // Find the reverb plugin and create automation component
    if (auto reverbPlugin = EngineHelpers::getPluginFromRack(edit, tracktion::engine::ReverbPlugin::xmlTypeName))
    {
        reverbAutomationComponent = std::make_unique<PluginAutomationComponent>(edit, reverbPlugin.get());
        pluginAutomationContainer.addAndMakeVisible(*reverbAutomationComponent);
    }

    // Find the delay plugin and create automation component
    if (auto delayPlugin = EngineHelpers::getPluginFromRack(edit, AutoDelayPlugin::xmlTypeName))
    {
        delayAutomationComponent = std::make_unique<PluginAutomationComponent>(edit, delayPlugin.get());
        pluginAutomationContainer.addAndMakeVisible(*delayAutomationComponent);
    }

    // Find the phaser plugin and create automation component
    if (auto phaserPlugin = EngineHelpers::getPluginFromRack(edit, AutoPhaserPlugin::xmlTypeName))
    {
        phaserAutomationComponent = std::make_unique<PluginAutomationComponent>(edit, phaserPlugin.get());
        pluginAutomationContainer.addAndMakeVisible(*phaserAutomationComponent);
    }

    // Find the flanger plugin and create automation component
    if (auto flangerPlugin = EngineHelpers::getPluginFromRack(edit, FlangerPlugin::xmlTypeName))
    {
        flangerAutomationComponent = std::make_unique<PluginAutomationComponent>(edit, flangerPlugin.get());
        pluginAutomationContainer.addAndMakeVisible(*flangerAutomationComponent);
    }

    // Register as automation listener
    edit.getAutomationRecordManager().addListener(this);
}

TransportComponent::~TransportComponent()
{
    // First stop the timer to prevent any callbacks
    stopTimer();
    
    // Remove automation listener
    edit.getAutomationRecordManager().removeListener(this);
}

void TransportComponent::timerCallback()
{
    updatePlayheadPosition();

    // Force thumbnail redraw during playback
    if (transport.isPlaying())
        repaint();
}

void TransportComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Draw background
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // Draw waveform area - fixed at 60 pixels
    auto waveformBounds = bounds.removeFromTop(60);
    g.setColour (juce::Colours::darkgrey.darker (0.7f));
    g.fillRect (waveformBounds);

    // Create drawing bounds
    auto drawBounds = waveformBounds.reduced (2);
    
    // Draw waveform if we have a clip
    if (currentClip != nullptr)
    {
        auto clipLength = currentClip->getPosition().getLength().inSeconds();
        auto sourceLength = currentClip->getSourceLength().inSeconds();
        auto timeStretchRatio = clipLength / sourceLength;  // This gives us the time-stretch ratio
        
        // Calculate visible time range in source time domain
        auto visibleTimeStart = (sourceLength * scrollPosition) * timeStretchRatio;
        auto visibleTimeEnd = visibleTimeStart + ((sourceLength / zoomLevel) * timeStretchRatio);
        
        auto timeRange = tracktion::TimeRange (
            tracktion::TimePosition::fromSeconds (visibleTimeStart),
            tracktion::TimePosition::fromSeconds (visibleTimeEnd));
        
        if (sourceLength > 0.0)
        {
            // Enable anti-aliasing
            g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);

            // Calculate gains from clip properties
            const auto gain = currentClip->getGain();
            const auto pan = thumbnail.getNumChannels() == 1 ? 0.0f : currentClip->getPan();
            const float pv = pan * gain;
            const float leftGain = (gain - pv);
            const float rightGain = (gain + pv);
            
            // Draw waveform first (so beat markers appear on top)
            g.setColour (juce::Colours::lime.withAlpha (0.7f));
            
            if (thumbnail.getTotalLength() > 0.0)
            {
                // Get crossfader parameter
                tracktion::engine::AutomatableParameter* crossfaderParam = nullptr;
                if (auto chopPlugin = EngineHelpers::getPluginFromMasterTrack(edit, ChopPlugin::xmlTypeName))
                {
                    if (auto* plugin = chopPlugin.get())
                    {
                        crossfaderParam = plugin->getAutomatableParameterByID("crossfader");
                    }
                }

                // Instead of drawing channels separately, draw a combined view
                float maxGain = jmax(leftGain, rightGain);

                if (crossfaderParam != nullptr)
                {
                    // Calculate visible time range
                    auto visibleTimeStartSource = visibleTimeStart / timeStretchRatio;  // Convert to source time domain
                    auto visibleTimeEndSource = visibleTimeEnd / timeStretchRatio;      // Convert to source time domain
                    
                    // Draw in small segments to capture crossfader changes
                    const int numSegments = drawBounds.getWidth();
                    const double timePerSegment = (visibleTimeEndSource - visibleTimeStartSource) / numSegments;
                    
                    for (int i = 0; i < numSegments; i++)
                    {
                        auto segmentTime = visibleTimeStartSource + (i * timePerSegment);
                        auto segmentEndTime = segmentTime + timePerSegment;
                        
                        // Get crossfader value at this time (in source time domain)
                        auto crossfaderValue = crossfaderParam->getCurve().getValueAt(tracktion::TimePosition::fromSeconds(segmentTime * timeStretchRatio));
                        
                        // Create segment bounds
                        auto segmentBounds = drawBounds.withWidth(1).withX(drawBounds.getX() + i);
                        
                        // Set color based on crossfader value
                        juce::Colour waveformColor = crossfaderValue >= 0.5f ? juce::Colours::purple : juce::Colours::lime;
                        
                        // Create gradient for this segment
                        g.setGradientFill(juce::ColourGradient(
                            waveformColor.withAlpha(0.8f),
                            segmentBounds.getTopLeft().toFloat(),
                            waveformColor.withAlpha(0.3f),
                            segmentBounds.getBottomLeft().toFloat(),
                            false));
                        
                        // Draw this segment in source time domain
                        thumbnail.drawChannels(g, 
                                            segmentBounds,
                                            tracktion::TimeRange(tracktion::TimePosition::fromSeconds(segmentTime),
                                                               tracktion::TimePosition::fromSeconds(segmentEndTime)),
                                            maxGain);
                    }
                    
                    // Add a subtle outline
                    g.setColour(juce::Colours::grey.withAlpha(0.4f));
                    g.drawRect(drawBounds.toFloat(), 1.0f);
                }
                else
                {
                    // Fallback if no crossfader parameter - draw in source time domain
                    g.setGradientFill(juce::ColourGradient(
                        juce::Colours::lime.withAlpha(0.8f),
                        drawBounds.getTopLeft().toFloat(),
                        juce::Colours::lime.withAlpha(0.3f),
                        drawBounds.getBottomLeft().toFloat(),
                        false));
                        
                    auto sourceTimeRange = tracktion::TimeRange(
                        tracktion::TimePosition::fromSeconds(visibleTimeStart / timeStretchRatio),
                        tracktion::TimePosition::fromSeconds(visibleTimeEnd / timeStretchRatio));
                        
                    thumbnail.drawChannels(g, drawBounds, sourceTimeRange, maxGain);
                }
            }
            else
            {
                // ... existing code ...
            }
        }
        else
        {
            // ... existing code ...
        }

        // Draw center line
        g.setColour (juce::Colours::grey.withAlpha (0.3f));
        g.drawHorizontalLine (drawBounds.getCentreY(),
            drawBounds.getX(),
            drawBounds.getRight());

        // Draw beat markers
        auto& tempoSequence = edit.tempoSequence;
        auto position = createPosition (tempoSequence);

        // Find the first beat before our visible range
        position.set (tracktion::TimePosition::fromSeconds (timeRange.getStart().inSeconds()));
        auto currentBarsBeats = position.getBarsBeats();
        auto beatTime = timeRange.getStart().inSeconds() - (currentBarsBeats.beats.inBeats() * 60.0 / position.getTempo());

        // Draw all beat markers
        for (double time = beatTime; time <= timeRange.getEnd().inSeconds(); time += 60.0 / position.getTempo())
        {
            if (time >= timeRange.getStart().inSeconds() && time <= timeRange.getEnd().inSeconds()) // Only draw if in visible range
            {
                auto beatX = drawBounds.getX() + ((time - timeRange.getStart().inSeconds()) / (timeRange.getEnd().inSeconds() - timeRange.getStart().inSeconds())) * drawBounds.getWidth();

                position.set (tracktion::TimePosition::fromSeconds (time));
                auto barsBeats = position.getBarsBeats();

                // Bar starts get a brighter color
                g.setColour (barsBeats.beats.inBeats() == 0
                                 ? juce::Colours::white.withAlpha (0.4f)
                                 : juce::Colours::white.withAlpha (0.2f));

                g.drawVerticalLine (static_cast<int> (beatX),
                    drawBounds.getY(),
                    drawBounds.getBottom());
            }
        }
    }
    else
    {
        // ... existing code ...
    }
}

void TransportComponent::resized()
{
    auto bounds = getLocalBounds();
    
    // Constants for layout
    const int controlBarHeight = 26;  // Fixed control bar height
    const int crossfaderHeight = 25;  // Fixed crossfader height
    const int thumbnailHeight = 60;   // Fixed thumbnail height
    
    // Create main FlexBox for vertical layout of all components
    juce::FlexBox mainFlex;
    mainFlex.flexDirection = juce::FlexBox::Direction::column;
    mainFlex.flexWrap = juce::FlexBox::Wrap::noWrap;
    mainFlex.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    mainFlex.alignItems = juce::FlexBox::AlignItems::stretch;
    
    // 1. Add transport bar
    mainFlex.items.add(juce::FlexItem(transportBar).withHeight(controlBarHeight).withMargin(juce::FlexItem::Margin(0, 5, 0, 5)));
    
    // 2. Add thumbnail section
    mainFlex.items.add(juce::FlexItem(bounds.getWidth(), thumbnailHeight).withMargin(juce::FlexItem::Margin(0, 0, 0, 0)));
    
    // 3. Add crossfader automation lane
    if (crossfaderAutomationLane != nullptr)
    {
        mainFlex.items.add(juce::FlexItem(*crossfaderAutomationLane).withHeight(crossfaderHeight).withMargin(juce::FlexItem::Margin(1, 0, 1, 0)));
    }
    
    // 4. Add plugin automation viewport
    mainFlex.items.add(juce::FlexItem(pluginAutomationViewport).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0, 0, 0, 0)));
    
    // Perform the main layout
    mainFlex.performLayout(bounds);
    
    // Setup plugin automation container layout
    juce::FlexBox pluginFlex;
    pluginFlex.flexDirection = juce::FlexBox::Direction::column;
    pluginFlex.flexWrap = juce::FlexBox::Wrap::noWrap;
    
    // Add automation components to plugin flex layout
    if (reverbAutomationComponent != nullptr)
        pluginFlex.items.add(juce::FlexItem(*reverbAutomationComponent).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0, 0, 1, 0)));
    
    if (delayAutomationComponent != nullptr)
        pluginFlex.items.add(juce::FlexItem(*delayAutomationComponent).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0, 0, 1, 0)));
    
    if (phaserAutomationComponent != nullptr)
        pluginFlex.items.add(juce::FlexItem(*phaserAutomationComponent).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0, 0, 1, 0)));
    
    if (flangerAutomationComponent != nullptr)
        pluginFlex.items.add(juce::FlexItem(*flangerAutomationComponent).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0, 0, 1, 0)));
    
    // Calculate total height for plugin container
    auto totalHeight = pluginAutomationViewport.getHeight() * 4;
    
    // Set container bounds and perform plugin layout
    pluginAutomationContainer.setBounds(0, 0, pluginAutomationViewport.getWidth(), totalHeight);
    pluginFlex.performLayout(pluginAutomationContainer.getBounds());
    
    // Update playhead if it exists
    if (playhead != nullptr)
    {
        playhead->setRectangle(juce::Rectangle<float>(2.0f, (float)bounds.getY(), 2.0f, (float)thumbnailHeight));
        updatePlayheadPosition();
    }
}

void TransportComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    // if (source == &(edit.getTransport()))
    // {
    //     updateTransportState();
    //     repaint();
    // }
}

void TransportComponent::updatePlayheadPosition()
{
    if (playhead != nullptr)
    {
        auto bounds = getLocalBounds();
        auto waveformBounds = bounds.removeFromTop(60).reduced(2);

        // Get current position from transport (this is already in the correct time domain)
        auto currentPosition = transport.getPosition().inSeconds();

        if (currentClip != nullptr)
        {
            auto sourceLength = currentClip->getPosition().getLength().inSeconds();

            // Get current tempo information
            auto& tempoSequence = edit.tempoSequence;
            auto position = createPosition(tempoSequence);
            position.set(transport.getPosition());
            auto currentTempo = position.getTempo();

            // Calculate visible time range in source time domain first
            auto visibleTimeStart = sourceLength * scrollPosition;
            auto visibleTimeEnd = visibleTimeStart + (sourceLength / zoomLevel);

            // Calculate normalized position directly in source time domain
            auto normalizedPosition = (currentPosition - visibleTimeStart) / (visibleTimeEnd - visibleTimeStart);

            // Only show playhead if it's in the visible range
            if (currentPosition >= visibleTimeStart && currentPosition <= visibleTimeEnd)
            {
                auto playheadX = waveformBounds.getX() + (normalizedPosition * waveformBounds.getWidth());
                playhead->setVisible(true);
                playhead->setTopLeftPosition(static_cast<int>(playheadX), waveformBounds.getY());
            }
            else
            {
                playhead->setVisible(false);
            }
        }
        else
        {
            playhead->setVisible(false);
        }
    }
}

void TransportComponent::updateThumbnail()
{
    auto audioTracks = te::getAudioTracks(edit);
    currentClip = nullptr; // Reset current clip reference

    for (auto track : audioTracks)
    {
        auto clips = track->getClips();

        for (auto clip : clips)
        {
            if (auto* waveClip = dynamic_cast<tracktion::engine::WaveAudioClip*>(clip))
            {
                auto audioFile = waveClip->getAudioFile();
                
                if (audioFile.isValid())
                {
                    currentClip = waveClip;
                    thumbnail.setNewFile(audioFile);
                    
                    // Update automation lanes with the clip reference
                    if (crossfaderAutomationLane != nullptr)
                        crossfaderAutomationLane->setClip(currentClip);
                    if (reverbAutomationComponent != nullptr)
                        reverbAutomationComponent->setClip(currentClip);
                    if (delayAutomationComponent != nullptr)
                        delayAutomationComponent->setClip(currentClip);
                    if (phaserAutomationComponent != nullptr)
                        phaserAutomationComponent->setClip(currentClip);
                    if (flangerAutomationComponent != nullptr)
                        flangerAutomationComponent->setClip(currentClip);
                        
                    repaint();
                    break;
                }
            }
        }
    }
}

void TransportComponent::mouseDown (juce::MouseEvent const& event)
{
    auto bounds = getLocalBounds();
    auto waveformBounds = bounds.removeFromTop(60);

    if (waveformBounds.contains(event.getPosition()))
    {
        if (currentClip != nullptr)
        {
            // Calculate normalized position from click
            auto clickX = event.position.x - waveformBounds.getX();
            auto normalizedPosition = (clickX / waveformBounds.getWidth()) / zoomLevel + scrollPosition;
            normalizedPosition = juce::jlimit(0.0, 1.0, normalizedPosition);

            auto sourceLength = currentClip->getPosition().getLength().inSeconds();
            auto newPosition = (normalizedPosition * sourceLength);

            transport.setPosition(tracktion::TimePosition::fromSeconds(newPosition));
            // updateTransportState();
        }
    }
}

void TransportComponent::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    auto bounds = getLocalBounds();
    auto waveformBounds = bounds.removeFromTop(60);

    if (waveformBounds.contains(event.getPosition()))
    {
        // Handle horizontal scrolling with shift key or horizontal wheel
        if (event.mods.isShiftDown() || wheel.deltaX != 0.0f)
        {
            auto delta = wheel.deltaX != 0.0f ? wheel.deltaX : wheel.deltaY;
            auto newScrollPos = scrollPosition - (delta * 0.1);
            setScrollPosition(newScrollPos);
        }
        // Handle zooming
        else if (wheel.deltaY != 0.0f)
        {
            // Calculate zoom factor based on wheel direction
            auto zoomFactor = wheel.deltaY > 0 ? 0.9 : 1.1; // Inverted for more natural feel
            
            // Calculate the mouse position as a proportion of the visible width
            auto mouseXProportion = (event.position.x - waveformBounds.getX()) / (float)waveformBounds.getWidth();
            
            // Calculate the absolute time position under the mouse
            auto timeUnderMouse = (scrollPosition + (mouseXProportion / zoomLevel));
            
            // Calculate new zoom level
            auto newZoomLevel = juce::jlimit(minZoom, maxZoom, zoomLevel * zoomFactor);
            
            // Calculate new scroll position that keeps the time under mouse at the same screen position
            auto newScrollPos = timeUnderMouse - (mouseXProportion / newZoomLevel);
            
            // Apply the changes
            setZoomLevel(newZoomLevel);
            setScrollPosition(newScrollPos);
        }
    }
}

void TransportComponent::setZoomLevel(double newLevel)
{
    zoomLevel = juce::jlimit(minZoom, maxZoom, newLevel);
    
    // Update automation lanes
    if (crossfaderAutomationLane != nullptr)
        crossfaderAutomationLane->setZoomLevel(zoomLevel);
    if (reverbAutomationComponent != nullptr)
        reverbAutomationComponent->setZoomLevel(zoomLevel);
    if (delayAutomationComponent != nullptr)
        delayAutomationComponent->setZoomLevel(zoomLevel);
    if (phaserAutomationComponent != nullptr)
        phaserAutomationComponent->setZoomLevel(zoomLevel);
    if (flangerAutomationComponent != nullptr)
        flangerAutomationComponent->setZoomLevel(zoomLevel);
        
    repaint();
}

void TransportComponent::setScrollPosition(double newPosition)
{
    // Limit scrolling based on zoom level
    auto maxScroll = getMaxScrollPosition();
    scrollPosition = juce::jlimit(0.0, maxScroll, newPosition);
    
    // Update automation lanes
    if (crossfaderAutomationLane != nullptr)
        crossfaderAutomationLane->setScrollPosition(scrollPosition);
    if (reverbAutomationComponent != nullptr)
        reverbAutomationComponent->setScrollPosition(scrollPosition);
    if (delayAutomationComponent != nullptr)
        delayAutomationComponent->setScrollPosition(scrollPosition);
    if (phaserAutomationComponent != nullptr)
        phaserAutomationComponent->setScrollPosition(scrollPosition);
    if (flangerAutomationComponent != nullptr)
        flangerAutomationComponent->setScrollPosition(scrollPosition);
        
    repaint();
}

double TransportComponent::getMaxScrollPosition() const
{
    return zoomLevel > 1.0 ? 1.0 - (1.0 / zoomLevel) : 0.0;
}

void TransportComponent::deleteSelectedChopRegion()
{
    if (crossfaderAutomationLane != nullptr)
    {
        crossfaderAutomationLane->deleteSelectedRegion();
    }
}
